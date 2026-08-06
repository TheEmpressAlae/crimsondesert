#include <Windows.h>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <share.h>
#include <cwchar>
#include <string>

#include "game_bridge.h"

namespace {

using marker_teleport::GameBridge;
using marker_teleport::TeleportRequest;
using marker_teleport::TeleportResult;

constexpr DWORD kPollMilliseconds = 25;
constexpr DWORD kDebounceMilliseconds = 250;

HMODULE g_module{};
std::atomic_bool g_stop{false};
GameBridge g_game;
std::wstring g_directory;
FILE* g_log{};

struct Config {
    bool enabled{true};
    bool logEnabled{true};
    int teleportKey{VK_F10};
    int reloadKey{VK_F11};
    float fallbackHeight{1200.0F};
    std::uint32_t invulnerabilitySeconds{10};
};

Config g_config;

std::wstring PathFor(const wchar_t* name) {
    return g_directory + L"\\" + name;
}

void Log(const char* text) {
    if (g_log != nullptr) {
        std::fputs(text, g_log);
        std::fflush(g_log);
    }
}

void LogTeleportResult(TeleportResult result) {
    switch (result) {
    case TeleportResult::Success: Log("Teleport result: success\n"); break;
    case TeleportResult::NotReady: Log("Teleport result: not ready\n"); break;
    case TeleportResult::NoPlayer: Log("Teleport result: no player\n"); break;
    case TeleportResult::NoMarker: Log("Teleport result: no marker\n"); break;
    case TeleportResult::UnsafeContext: Log("Teleport result: unsafe context\n"); break;
    case TeleportResult::InvalidCoordinates: Log("Teleport result: invalid coordinates\n"); break;
    case TeleportResult::WriteFailed: Log("Teleport result: position write failed\n"); break;
    }
}

int ParseFunctionKey(const wchar_t* value, int fallback) {
    if (value == nullptr || (value[0] != L'F' && value[0] != L'f')) {
        return fallback;
    }
    wchar_t* end{};
    const long number = std::wcstol(value + 1, &end, 10);
    if (end == value + 1 || *end != L'\0' || number < 1 || number > 24) {
        return fallback;
    }
    return VK_F1 + static_cast<int>(number - 1);
}

void LoadConfig() {
    const std::wstring path = PathFor(L"MarkerTeleport.ini");
    g_config.enabled = GetPrivateProfileIntW(L"General", L"Enabled", 1, path.c_str()) != 0;
    g_config.logEnabled = GetPrivateProfileIntW(L"General", L"LogEnabled", 1, path.c_str()) != 0;

    wchar_t value[64]{};
    GetPrivateProfileStringW(L"General", L"ReloadKey", L"F11", value, 64, path.c_str());
    g_config.reloadKey = ParseFunctionKey(value, VK_F11);
    GetPrivateProfileStringW(L"Teleport", L"Hotkey", L"F10", value, 64, path.c_str());
    g_config.teleportKey = ParseFunctionKey(value, VK_F10);
    GetPrivateProfileStringW(L"Teleport", L"FallbackHeight", L"1200", value, 64, path.c_str());
    g_config.fallbackHeight = std::clamp(std::wcstof(value, nullptr), -100000.0F, 100000.0F);
    const int invulnerabilitySeconds = static_cast<int>(
        GetPrivateProfileIntW(L"Teleport", L"InvulnerabilitySeconds", 10, path.c_str()));
    g_config.invulnerabilitySeconds = static_cast<std::uint32_t>(
        std::clamp(invulnerabilitySeconds, 0, 300));
}

bool PressedOnce(int virtualKey, bool& previous) {
    const bool down = (GetAsyncKeyState(virtualKey) & 0x8000) != 0;
    const bool pressed = down && !previous;
    previous = down;
    return pressed;
}

DWORD WINAPI Worker(void*) {
    wchar_t modulePath[MAX_PATH]{};
    if (GetModuleFileNameW(g_module, modulePath, MAX_PATH) == 0) {
        return 0;
    }
    if (wchar_t* slash = std::wcsrchr(modulePath, L'\\'); slash != nullptr) {
        *slash = L'\0';
    }
    g_directory = modulePath;
    LoadConfig();

    if (g_config.logEnabled) {
        g_log = _wfsopen(PathFor(L"MarkerTeleport.log").c_str(), L"w",
                         _SH_DENYNO);
    }
    Log("MarkerTeleport v1.0.0 clean-room build starting\n");

    if (!g_game.Initialize()) {
        Log("FAIL-CLOSED: current-build game interfaces are unresolved; teleport is disabled\n");
    } else {
        Log("READY: current-build interfaces resolved; teleport input is active\n");
    }

    bool teleportWasDown = false;
    bool reloadWasDown = false;
    ULONGLONG lastTeleport = 0;

    while (!g_stop.load(std::memory_order_relaxed)) {
        const ULONGLONG now = GetTickCount64();
        if (PressedOnce(g_config.reloadKey, reloadWasDown)) {
            LoadConfig();
            Log("Configuration reloaded\n");
        }

        if (g_config.enabled && g_game.Ready() &&
            PressedOnce(g_config.teleportKey, teleportWasDown) &&
            now - lastTeleport >= kDebounceMilliseconds) {
            lastTeleport = now;
            const TeleportRequest request{
                g_config.fallbackHeight,
                g_config.invulnerabilitySeconds * 1000U,
            };
            LogTeleportResult(g_game.TeleportToMarker(request));
        } else if (!g_config.enabled || !g_game.Ready()) {
            teleportWasDown = (GetAsyncKeyState(g_config.teleportKey) & 0x8000) != 0;
        }

        g_game.ServiceProtectionExpiry(now);
        Sleep(kPollMilliseconds);
    }

    g_game.Shutdown();
    if (g_log != nullptr) {
        std::fclose(g_log);
        g_log = nullptr;
    }
    return 0;
}

bool IsTargetProcess() {
    wchar_t path[MAX_PATH]{};
    if (GetModuleFileNameW(nullptr, path, MAX_PATH) == 0) {
        return false;
    }
    const wchar_t* name = std::wcsrchr(path, L'\\');
    name = name == nullptr ? path : name + 1;
    return _wcsicmp(name, L"CrimsonDesert.exe") == 0;
}

}  // namespace

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID reserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        g_module = module;
        DisableThreadLibraryCalls(module);
        if (IsTargetProcess()) {
            if (HANDLE thread = CreateThread(nullptr, 0, Worker, nullptr, 0, nullptr); thread != nullptr) {
                CloseHandle(thread);
            }
        }
    } else if (reason == DLL_PROCESS_DETACH && reserved == nullptr) {
        g_stop.store(true, std::memory_order_relaxed);
    }
    return TRUE;
}
