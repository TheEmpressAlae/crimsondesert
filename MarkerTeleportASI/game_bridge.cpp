#include "game_bridge.h"

#include <Windows.h>
#include <TlHelp32.h>

#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <vector>

namespace marker_teleport {
namespace {

constexpr std::array<std::uint8_t, 11> kPlayerPattern{
    0x48, 0x8B, 0x06, 0xC5, 0xF8, 0x11, 0x88, 0xB0, 0x01, 0x00, 0x00,
};
constexpr std::array<std::uint8_t, 14> kMarkerPattern{
    0xC5, 0xFB, 0x10, 0x07, 0xC5, 0xFB, 0x11,
    0x02, 0x8B, 0x47, 0x08, 0x89, 0x42, 0x08,
};
constexpr std::array<std::uint8_t, 4> kOriginPrefix{0xC5, 0xF8, 0x5C, 0x05};
constexpr std::array<std::uint8_t, 7> kProtectionPattern{
    0x48, 0x8B, 0x46, 0x08, 0x48, 0x89, 0xF1,
};

constexpr std::size_t kExpectedMarkerMatches = 5;
constexpr std::size_t kExpectedOriginMatches = 9;
constexpr float kCoordinateLimit = 1.0e9F;
constexpr float kDestinationLift = 10.0F;

struct Vec3 {
    float x{};
    float y{};
    float z{};
};

struct CandidateSlot {
    alignas(8) std::uint64_t xyBits{};
    std::uint32_t zBits{};
    alignas(4) std::uint32_t writer{};
    std::uint32_t valid{};
};

struct InlineHook {
    std::uintptr_t target{};
    std::size_t length{};
    std::array<std::uint8_t, 16> original{};
    void* stub{};
};

alignas(8) std::uintptr_t g_player{};
std::array<CandidateSlot, kExpectedMarkerMatches> g_candidates{};
std::atomic<std::uint64_t> g_protectFlag{0};
std::uintptr_t g_originAddress{};
int g_cachedCandidate{-1};
std::vector<InlineHook> g_hooks;

class ThreadSuspender final {
public:
    bool SuspendOthersAvoiding(std::uintptr_t rangeStart, std::size_t rangeLength) {
        for (unsigned attempt = 0; attempt < 64; ++attempt) {
            if (!CollectAndSuspend()) {
                return false;
            }

            bool conflict = false;
            if (rangeLength != 0) {
                for (HANDLE thread : handles_) {
                    CONTEXT context{};
                    context.ContextFlags = CONTEXT_CONTROL;
                    if (!GetThreadContext(thread, &context)) {
                        conflict = true;
                        break;
                    }
                    const std::uintptr_t rip = static_cast<std::uintptr_t>(context.Rip);
                    if (rip >= rangeStart && rip < rangeStart + rangeLength) {
                        conflict = true;
                        break;
                    }
                }
            }
            if (!conflict) {
                return true;
            }
            ResumeAll();
            Sleep(1);
        }
        return false;
    }

    ~ThreadSuspender() {
        ResumeAll();
    }

private:
    bool CollectAndSuspend() {
        const DWORD processId = GetCurrentProcessId();
        const DWORD currentThreadId = GetCurrentThreadId();
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
        if (snapshot == INVALID_HANDLE_VALUE) {
            return false;
        }

        THREADENTRY32 entry{sizeof(entry)};
        if (Thread32First(snapshot, &entry)) {
            do {
                if (entry.th32OwnerProcessID != processId || entry.th32ThreadID == currentThreadId) {
                    continue;
                }
                HANDLE thread = OpenThread(
                    THREAD_SUSPEND_RESUME | THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION,
                    FALSE, entry.th32ThreadID);
                if (thread == nullptr) {
                    const DWORD error = GetLastError();
                    if (error == ERROR_INVALID_PARAMETER) {
                        continue;  // Thread exited after the snapshot.
                    }
                    CloseHandle(snapshot);
                    ResumeAll();
                    return false;
                }
                handles_.push_back(thread);
            } while (Thread32Next(snapshot, &entry));
        }
        CloseHandle(snapshot);

        for (HANDLE thread : handles_) {
            if (SuspendThread(thread) == static_cast<DWORD>(-1)) {
                ResumeAll();
                return false;
            }
            ++suspendedCount_;
        }
        return true;
    }

    void ResumeAll() {
        while (suspendedCount_ != 0) {
            --suspendedCount_;
            ResumeThread(handles_[suspendedCount_]);
        }
        for (HANDLE thread : handles_) {
            CloseHandle(thread);
        }
        handles_.clear();
    }

    std::vector<HANDLE> handles_;
    std::size_t suspendedCount_{};
};

template <std::size_t N>
std::vector<std::uintptr_t> ScanExecutableSections(const std::array<std::uint8_t, N>& pattern) {
    std::vector<std::uintptr_t> matches;
    auto* base = reinterpret_cast<std::uint8_t*>(GetModuleHandleW(nullptr));
    if (base == nullptr) {
        return matches;
    }

    const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) {
        return matches;
    }
    const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS*>(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) {
        return matches;
    }

    const IMAGE_SECTION_HEADER* section = IMAGE_FIRST_SECTION(nt);
    for (unsigned index = 0; index < nt->FileHeader.NumberOfSections; ++index, ++section) {
        if ((section->Characteristics & IMAGE_SCN_MEM_EXECUTE) == 0) {
            continue;
        }
        const std::size_t size = section->Misc.VirtualSize;
        if (size < N) {
            continue;
        }
        const auto* begin = base + section->VirtualAddress;
        const auto* end = begin + size - N + 1;
        for (const auto* cursor = begin; cursor < end; ++cursor) {
            if (std::memcmp(cursor, pattern.data(), N) == 0) {
                matches.push_back(reinterpret_cast<std::uintptr_t>(cursor));
            }
        }
    }
    return matches;
}

void* AllocateNear(std::uintptr_t target, std::size_t size) {
    SYSTEM_INFO info{};
    GetSystemInfo(&info);
    const std::uintptr_t granularity = info.dwAllocationGranularity;
    const std::uintptr_t aligned = target & ~(granularity - 1);
    constexpr std::uintptr_t kMaxDistance = 0x7FFF0000ULL;

    for (std::uintptr_t distance = 0; distance <= kMaxDistance; distance += granularity) {
        const std::uintptr_t high = aligned + distance;
        if (high >= aligned) {
            if (void* block = VirtualAlloc(reinterpret_cast<void*>(high), size,
                                           MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE)) {
                return block;
            }
        }
        if (distance != 0 && aligned > distance) {
            const std::uintptr_t low = aligned - distance;
            if (void* block = VirtualAlloc(reinterpret_cast<void*>(low), size,
                                           MEM_RESERVE | MEM_COMMIT, PAGE_EXECUTE_READWRITE)) {
                return block;
            }
        }
    }
    return nullptr;
}

void Append(std::vector<std::uint8_t>& code, std::initializer_list<std::uint8_t> bytes) {
    code.insert(code.end(), bytes.begin(), bytes.end());
}

template <typename T>
void AppendValue(std::vector<std::uint8_t>& code, T value) {
    const auto* bytes = reinterpret_cast<const std::uint8_t*>(&value);
    code.insert(code.end(), bytes, bytes + sizeof(T));
}

bool AppendRel32Jump(std::vector<std::uint8_t>& code, std::uintptr_t codeBase,
                     std::uintptr_t destination) {
    const std::uintptr_t instructionEnd = codeBase + code.size() + 5;
    const auto delta = static_cast<std::int64_t>(destination) - static_cast<std::int64_t>(instructionEnd);
    if (delta < std::numeric_limits<std::int32_t>::min() ||
        delta > std::numeric_limits<std::int32_t>::max()) {
        return false;
    }
    code.push_back(0xE9);
    AppendValue(code, static_cast<std::int32_t>(delta));
    return true;
}

bool PatchTarget(InlineHook& hook, const std::vector<std::uint8_t>& stubCode,
                 const std::uint8_t* expectedBytes) {
    if (hook.length < 5 || hook.length > hook.original.size() || hook.stub == nullptr) {
        return false;
    }
    std::memcpy(hook.original.data(), reinterpret_cast<const void*>(hook.target), hook.length);
    std::memcpy(hook.stub, stubCode.data(), stubCode.size());

    const auto delta = static_cast<std::int64_t>(reinterpret_cast<std::uintptr_t>(hook.stub)) -
                       static_cast<std::int64_t>(hook.target + 5);
    if (delta < std::numeric_limits<std::int32_t>::min() ||
        delta > std::numeric_limits<std::int32_t>::max()) {
        return false;
    }

    ThreadSuspender suspension;
    if (!suspension.SuspendOthersAvoiding(hook.target, hook.length)) {
        return false;
    }
    if (std::memcmp(reinterpret_cast<const void*>(hook.target), expectedBytes,
                    hook.length) != 0) {
        return false;
    }
    DWORD oldProtect{};
    if (!VirtualProtect(reinterpret_cast<void*>(hook.target), hook.length,
                        PAGE_EXECUTE_READWRITE, &oldProtect)) {
        return false;
    }
    auto* target = reinterpret_cast<std::uint8_t*>(hook.target);
    target[0] = 0xE9;
    const auto relative = static_cast<std::int32_t>(delta);
    std::memcpy(target + 1, &relative, sizeof(relative));
    std::fill(target + 5, target + hook.length, static_cast<std::uint8_t>(0x90));
    FlushInstructionCache(GetCurrentProcess(), target, hook.length);
    DWORD ignored{};
    VirtualProtect(target, hook.length, oldProtect, &ignored);
    return true;
}

bool InstallPlayerHook(std::uintptr_t target) {
    InlineHook hook{target, 8};
    if (std::memcmp(reinterpret_cast<const void*>(target), kPlayerPattern.data() + 3,
                    hook.length) != 0) {
        return false;
    }
    hook.stub = AllocateNear(target, 128);
    if (hook.stub == nullptr) {
        return false;
    }
    const auto stubBase = reinterpret_cast<std::uintptr_t>(hook.stub);
    std::vector<std::uint8_t> code;
    Append(code, {0x48, 0xA3});                         // mov [abs], rax
    AppendValue(code, reinterpret_cast<std::uintptr_t>(&g_player));
    Append(code, {0xC5, 0xF8, 0x11, 0x88, 0xB0, 0x01, 0x00, 0x00});
    if (!AppendRel32Jump(code, stubBase, target + hook.length) ||
        !PatchTarget(hook, code, kPlayerPattern.data() + 3)) {
        VirtualFree(hook.stub, 0, MEM_RELEASE);
        return false;
    }
    g_hooks.push_back(hook);
    return true;
}

bool InstallMarkerHook(std::uintptr_t target, CandidateSlot& slot) {
    InlineHook hook{target, 7};
    if (std::memcmp(reinterpret_cast<const void*>(target), kMarkerPattern.data() + 4,
                    hook.length) != 0) {
        return false;
    }
    hook.stub = AllocateNear(target, 160);
    if (hook.stub == nullptr) {
        return false;
    }
    const auto stubBase = reinterpret_cast<std::uintptr_t>(hook.stub);
    std::vector<std::uint8_t> code;
    code.push_back(0x9C);                              // pushfq
    code.push_back(0x51);                              // push rcx
    Append(code, {0x41, 0x53});                        // push r11
    Append(code, {0x49, 0xBB});                        // mov r11, writer address
    AppendValue(code, reinterpret_cast<std::uintptr_t>(&slot.writer));
    Append(code, {0x31, 0xC0});                        // xor eax, eax
    Append(code, {0xB9, 0x01, 0x00, 0x00, 0x00});     // mov ecx, 1
    Append(code, {0xF0, 0x41, 0x0F, 0xB1, 0x0B});     // lock cmpxchg [r11], ecx
    const std::size_t jumpIfBusy = code.size();
    Append(code, {0x75, 0x00});                        // jne replay
    Append(code, {0x41, 0x5B});                        // pop r11
    code.push_back(0x59);                              // pop rcx
    code.push_back(0x9D);                              // popfq
    Append(code, {0x48, 0x8B, 0x07});                  // mov rax, [rdi]
    Append(code, {0x48, 0xA3});                        // mov [abs], rax
    AppendValue(code, reinterpret_cast<std::uintptr_t>(&slot.xyBits));
    Append(code, {0x8B, 0x47, 0x08});                  // mov eax, [rdi+8]
    code.push_back(0xA3);                              // mov [abs], eax
    AppendValue(code, reinterpret_cast<std::uintptr_t>(&slot.zBits));
    code.push_back(0x50);                              // push rax
    Append(code, {0xB8, 0x01, 0x00, 0x00, 0x00});     // mov eax, 1
    code.push_back(0xA3);                              // mov [abs], eax
    AppendValue(code, reinterpret_cast<std::uintptr_t>(&slot.valid));
    Append(code, {0x48, 0xB8});                        // mov rax, writer address
    AppendValue(code, reinterpret_cast<std::uintptr_t>(&slot.writer));
    Append(code, {0xC7, 0x00, 0x00, 0x00, 0x00, 0x00}); // mov dword ptr [rax], 0
    code.push_back(0x58);                              // pop rax
    Append(code, {0xC5, 0xFB, 0x11, 0x02});            // original store XY
    Append(code, {0x8B, 0x47, 0x08});                  // original load Z
    if (!AppendRel32Jump(code, stubBase, target + hook.length)) {
        VirtualFree(hook.stub, 0, MEM_RELEASE);
        return false;
    }
    const std::size_t busyReplay = code.size();
    Append(code, {0x41, 0x5B});                        // pop r11
    code.push_back(0x59);                              // pop rcx
    code.push_back(0x9D);                              // popfq
    Append(code, {0xC5, 0xFB, 0x11, 0x02});            // original store XY
    Append(code, {0x8B, 0x47, 0x08});                  // original load Z
    if (!AppendRel32Jump(code, stubBase, target + hook.length)) {
        VirtualFree(hook.stub, 0, MEM_RELEASE);
        return false;
    }
    const auto busyDelta = static_cast<std::int64_t>(busyReplay) -
                           static_cast<std::int64_t>(jumpIfBusy + 2);
    if (busyDelta < std::numeric_limits<std::int8_t>::min() ||
        busyDelta > std::numeric_limits<std::int8_t>::max()) {
        VirtualFree(hook.stub, 0, MEM_RELEASE);
        return false;
    }
    code[jumpIfBusy + 1] = static_cast<std::uint8_t>(static_cast<std::int8_t>(busyDelta));
    if (!PatchTarget(hook, code, kMarkerPattern.data() + 4)) {
        VirtualFree(hook.stub, 0, MEM_RELEASE);
        return false;
    }
    g_hooks.push_back(hook);
    return true;
}

bool InstallProtectionHook(std::uintptr_t target) {
    InlineHook hook{target, 7};
    if (std::memcmp(reinterpret_cast<const void*>(target), kProtectionPattern.data(),
                    hook.length) != 0) {
        return false;
    }
    hook.stub = AllocateNear(target, 160);
    if (hook.stub == nullptr) {
        return false;
    }
    const auto stubBase = reinterpret_cast<std::uintptr_t>(hook.stub);
    std::vector<std::uint8_t> code;
    code.push_back(0x9C);                              // pushfq
    Append(code, {0x48, 0xA1});                        // mov rax, [abs]
    AppendValue(code, reinterpret_cast<std::uintptr_t>(&g_protectFlag));
    Append(code, {0x48, 0x85, 0xC0});                  // test rax, rax
    const std::size_t jumpIfOff = code.size();
    Append(code, {0x74, 0x00});                        // jz replay
    Append(code, {0x80, 0x3E, 0x00});                  // cmp byte ptr [rsi], 0
    const std::size_t jumpIfBusy = code.size();
    Append(code, {0x75, 0x00});                        // jne replay
    Append(code, {0x48, 0x8B, 0x46, 0x18});            // mov rax, [rsi+18]
    Append(code, {0x48, 0x89, 0x46, 0x08});            // mov [rsi+8], rax
    const std::size_t replay = code.size();
    code[jumpIfOff + 1] = static_cast<std::uint8_t>(replay - (jumpIfOff + 2));
    code[jumpIfBusy + 1] = static_cast<std::uint8_t>(replay - (jumpIfBusy + 2));
    code.push_back(0x9D);                              // popfq
    Append(code, {0x48, 0x8B, 0x46, 0x08});            // original: mov rax,[rsi+8]
    Append(code, {0x48, 0x89, 0xF1});                  // original: mov rcx,rsi
    if (!AppendRel32Jump(code, stubBase, target + hook.length) ||
        !PatchTarget(hook, code, kProtectionPattern.data())) {
        VirtualFree(hook.stub, 0, MEM_RELEASE);
        return false;
    }
    g_hooks.push_back(hook);
    return true;
}

void RemoveHooks() {
    for (auto iterator = g_hooks.rbegin(); iterator != g_hooks.rend(); ++iterator) {
        ThreadSuspender suspension;
        if (!suspension.SuspendOthersAvoiding(iterator->target, iterator->length)) {
            continue;
        }
        auto* target = reinterpret_cast<std::uint8_t*>(iterator->target);
        std::int32_t relative{};
        if (target[0] != 0xE9) {
            continue;
        }
        std::memcpy(&relative, target + 1, sizeof(relative));
        if (iterator->target + 5 + relative != reinterpret_cast<std::uintptr_t>(iterator->stub)) {
            continue;
        }
        DWORD oldProtect{};
        if (VirtualProtect(reinterpret_cast<void*>(iterator->target), iterator->length,
                           PAGE_EXECUTE_READWRITE, &oldProtect)) {
            std::memcpy(reinterpret_cast<void*>(iterator->target), iterator->original.data(),
                        iterator->length);
            FlushInstructionCache(GetCurrentProcess(), reinterpret_cast<void*>(iterator->target),
                                  iterator->length);
            DWORD ignored{};
            VirtualProtect(reinterpret_cast<void*>(iterator->target), iterator->length,
                           oldProtect, &ignored);
        }
        // Stubs intentionally remain allocated until process exit. A suspended
        // thread may already be inside one and must be allowed to return safely.
    }
    g_hooks.clear();
}

bool SafeReadVec3(std::uintptr_t address, Vec3& value) {
    __try {
        value = *reinterpret_cast<const Vec3*>(address);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool SafeWriteDestination(std::uintptr_t player, const Vec3& value) {
    __try {
        *reinterpret_cast<Vec3*>(player + 0x90) = value;
        *reinterpret_cast<Vec3*>(player + 0x1A0) = value;
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool FiniteCoordinate(const Vec3& value) {
    if (!std::isfinite(value.x) || !std::isfinite(value.y) || !std::isfinite(value.z)) {
        return false;
    }
    if (std::abs(value.x) > kCoordinateLimit || std::abs(value.y) > kCoordinateLimit ||
        std::abs(value.z) > kCoordinateLimit) {
        return false;
    }
    return true;
}

bool ValidMarker(const Vec3& value) {
    return FiniteCoordinate(value) &&
           (value.x != 0.0F || value.y != 0.0F || value.z != 0.0F);
}

bool ReadCandidate(std::size_t index, Vec3& value) {
    CandidateSlot& slot = g_candidates[index];
    std::atomic_ref<std::uint32_t> writer(slot.writer);
    std::uint32_t expected = 0;
    if (!writer.compare_exchange_strong(expected, 1, std::memory_order_acquire,
                                        std::memory_order_relaxed)) {
        return false;
    }
    const bool valid = slot.valid != 0;
    const std::uint64_t xy = std::atomic_ref<std::uint64_t>(slot.xyBits).load(
        std::memory_order_relaxed);
    const std::uint32_t z = std::atomic_ref<std::uint32_t>(slot.zBits).load(
        std::memory_order_relaxed);
    writer.store(0, std::memory_order_release);
    if (!valid) {
        return false;
    }
    static_assert(sizeof(float) * 2 == sizeof(xy));
    std::memcpy(&value.x, &xy, sizeof(xy));
    std::memcpy(&value.z, &z, sizeof(z));
    return ValidMarker(value);
}

}  // namespace

bool GameBridge::Initialize() {
    ready_ = false;
    protectionReady_ = false;
    protectionDeadline_ = 0;
    std::atomic_ref<std::uintptr_t>(g_player).store(0, std::memory_order_relaxed);
    g_protectFlag.store(0, std::memory_order_relaxed);
    g_cachedCandidate = -1;
    for (CandidateSlot& slot : g_candidates) {
        std::atomic_ref<std::uint32_t>(slot.writer).store(0, std::memory_order_relaxed);
        std::atomic_ref<std::uint32_t>(slot.valid).store(0, std::memory_order_relaxed);
    }

    const auto players = ScanExecutableSections(kPlayerPattern);
    const auto markers = ScanExecutableSections(kMarkerPattern);
    const auto origins = ScanExecutableSections(kOriginPrefix);
    const auto protections = ScanExecutableSections(kProtectionPattern);
    if (players.size() != 1 || markers.size() != kExpectedMarkerMatches ||
        origins.size() != kExpectedOriginMatches) {
        return false;
    }

    std::uintptr_t origin{};
    for (const std::uintptr_t hit : origins) {
        std::int32_t displacement{};
        std::memcpy(&displacement, reinterpret_cast<const void*>(hit + 4), sizeof(displacement));
        const std::uintptr_t resolved = hit + 8 + displacement;
        if (origin == 0) {
            origin = resolved;
        } else if (resolved != origin) {
            return false;
        }
    }
    g_originAddress = origin;

    if (!InstallPlayerHook(players.front() + 3)) {
        RemoveHooks();
        return false;
    }
    for (std::size_t index = 0; index < markers.size(); ++index) {
        if (!InstallMarkerHook(markers[index] + 4, g_candidates[index])) {
            RemoveHooks();
            return false;
        }
    }
    if (protections.size() == 1) {
        protectionReady_ = InstallProtectionHook(protections.front());
    }

    ready_ = true;
    return true;
}

void GameBridge::Shutdown() {
    g_protectFlag.store(0, std::memory_order_release);
    // ASI loaders keep plugins resident for the lifetime of the process. Hooks
    // and their stubs intentionally remain installed until process exit.
    ready_ = false;
    protectionReady_ = false;
    protectionDeadline_ = 0;
}

bool GameBridge::Ready() const noexcept {
    return ready_;
}

TeleportResult GameBridge::TeleportToMarker(const TeleportRequest& request) {
    if (!ready_) {
        return TeleportResult::NotReady;
    }
    const std::uintptr_t player =
        std::atomic_ref<std::uintptr_t>(g_player).load(std::memory_order_acquire);
    if (player == 0) {
        return TeleportResult::NoPlayer;
    }

    Vec3 marker{};
    bool found = false;
    if (g_cachedCandidate >= 0) {
        found = ReadCandidate(static_cast<std::size_t>(g_cachedCandidate), marker);
    }
    for (std::size_t index = 0; !found && index < g_candidates.size(); ++index) {
        if (ReadCandidate(index, marker)) {
            g_cachedCandidate = static_cast<int>(index);
            found = true;
        }
    }
    if (!found) {
        return TeleportResult::NoMarker;
    }

    Vec3 origin{};
    if (g_originAddress == 0 || !SafeReadVec3(g_originAddress, origin) || !FiniteCoordinate(origin)) {
        return TeleportResult::InvalidCoordinates;
    }

    const bool usesFallbackHeight = marker.y == 0.0F;
    if (usesFallbackHeight &&
        (request.invulnerabilityMilliseconds == 0 || !protectionReady_)) {
        return TeleportResult::UnsafeContext;
    }
    const float height = usesFallbackHeight ? request.fallbackHeight : marker.y;
    const Vec3 destination{
        marker.x - origin.x,
        height + kDestinationLift,
        marker.z - origin.z,
    };
    if (!FiniteCoordinate(destination)) {
        return TeleportResult::InvalidCoordinates;
    }

    if (request.invulnerabilityMilliseconds != 0 && protectionReady_) {
        g_protectFlag.store(1, std::memory_order_release);
        protectionDeadline_ = GetTickCount64() + request.invulnerabilityMilliseconds;
    }
    if (!SafeWriteDestination(player, destination)) {
        g_protectFlag.store(0, std::memory_order_release);
        protectionDeadline_ = 0;
        return TeleportResult::WriteFailed;
    }
    return TeleportResult::Success;
}

void GameBridge::ServiceProtectionExpiry(std::uint64_t nowMilliseconds) {
    if (protectionDeadline_ != 0 && nowMilliseconds >= protectionDeadline_) {
        g_protectFlag.store(0, std::memory_order_release);
        protectionDeadline_ = 0;
    }
}

}  // namespace marker_teleport
