#pragma once

#include <Windows.h>

#include <cstddef>

namespace marker_teleport::key_config {
namespace detail {

struct NamedKey {
    const wchar_t* name;
    int virtualKey;
};

constexpr wchar_t UpperAscii(wchar_t value) noexcept {
    return value >= L'a' && value <= L'z' ? value - (L'a' - L'A') : value;
}

constexpr bool EqualToken(const wchar_t* value, const wchar_t* expected) noexcept {
    if (value == nullptr || expected == nullptr) {
        return false;
    }
    std::size_t cursor = 0;
    while (value[cursor] != L'\0' && expected[cursor] != L'\0') {
        if (UpperAscii(value[cursor]) != UpperAscii(expected[cursor])) {
            return false;
        }
        ++cursor;
    }
    return value[cursor] == L'\0' && expected[cursor] == L'\0';
}

constexpr bool ConsumePrefix(const wchar_t* value, const wchar_t* prefix,
                             std::size_t& cursor) noexcept {
    if (value == nullptr || prefix == nullptr) {
        return false;
    }
    cursor = 0;
    while (prefix[cursor] != L'\0') {
        if (value[cursor] == L'\0' ||
            UpperAscii(value[cursor]) != UpperAscii(prefix[cursor])) {
            return false;
        }
        ++cursor;
    }
    return true;
}

constexpr int ParseNumberedKey(const wchar_t* value, const wchar_t* prefix,
                               int minimum, int maximum, int virtualKeyBase,
                               int fallback) noexcept {
    std::size_t cursor{};
    if (!ConsumePrefix(value, prefix, cursor) || value[cursor] == L'\0') {
        return fallback;
    }

    int number = 0;
    while (value[cursor] >= L'0' && value[cursor] <= L'9') {
        number = number * 10 + static_cast<int>(value[cursor] - L'0');
        if (number > maximum) {
            return fallback;
        }
        ++cursor;
    }
    if (value[cursor] != L'\0' || number < minimum || number > maximum) {
        return fallback;
    }
    return virtualKeyBase + (number - minimum);
}

constexpr int HexDigit(wchar_t value) noexcept {
    if (value >= L'0' && value <= L'9') {
        return static_cast<int>(value - L'0');
    }
    const wchar_t upper = UpperAscii(value);
    return upper >= L'A' && upper <= L'F' ? static_cast<int>(upper - L'A') + 10 : -1;
}

constexpr int ParseHexVirtualKey(const wchar_t* value, int fallback) noexcept {
    std::size_t cursor{};
    if (!ConsumePrefix(value, L"VK:0X", cursor) &&
        !ConsumePrefix(value, L"VK_0X", cursor)) {
        return fallback;
    }
    if (value[cursor] == L'\0' || value[cursor + 1] == L'\0') {
        return fallback;
    }
    const int high = HexDigit(value[cursor]);
    const int low = HexDigit(value[cursor + 1]);
    if (high < 0 || low < 0 || value[cursor + 2] != L'\0') {
        return fallback;
    }
    const int virtualKey = high * 16 + low;
    return virtualKey >= 0x01 && virtualKey <= 0xFE ? virtualKey : fallback;
}

inline constexpr NamedKey kNamedKeys[]{
    // Mouse buttons.
    {L"MOUSE_LEFT", VK_LBUTTON}, {L"MOUSE1", VK_LBUTTON}, {L"LBUTTON", VK_LBUTTON},
    {L"MOUSE_RIGHT", VK_RBUTTON}, {L"MOUSE2", VK_RBUTTON}, {L"RBUTTON", VK_RBUTTON},
    {L"MOUSE_MIDDLE", VK_MBUTTON}, {L"MOUSE3", VK_MBUTTON}, {L"MBUTTON", VK_MBUTTON},
    {L"MOUSE_X1", VK_XBUTTON1}, {L"MOUSE4", VK_XBUTTON1}, {L"XBUTTON1", VK_XBUTTON1},
    {L"MOUSE_X2", VK_XBUTTON2}, {L"MOUSE5", VK_XBUTTON2}, {L"XBUTTON2", VK_XBUTTON2},

    // Editing, navigation, and system keys.
    {L"BACKSPACE", VK_BACK}, {L"BACK", VK_BACK}, {L"TAB", VK_TAB},
    {L"CLEAR", VK_CLEAR}, {L"ENTER", VK_RETURN}, {L"RETURN", VK_RETURN},
    {L"CANCEL", VK_CANCEL}, {L"PAUSE", VK_PAUSE},
    {L"CAPSLOCK", VK_CAPITAL}, {L"CAPITAL", VK_CAPITAL},
    {L"ESC", VK_ESCAPE}, {L"ESCAPE", VK_ESCAPE},
    {L"SPACE", VK_SPACE}, {L"SPACEBAR", VK_SPACE},
    {L"PAGEUP", VK_PRIOR}, {L"PRIOR", VK_PRIOR}, {L"PGUP", VK_PRIOR},
    {L"PAGEDOWN", VK_NEXT}, {L"NEXT", VK_NEXT}, {L"PGDN", VK_NEXT},
    {L"END", VK_END}, {L"HOME", VK_HOME},
    {L"LEFT", VK_LEFT}, {L"LEFTARROW", VK_LEFT}, {L"UP", VK_UP}, {L"UPARROW", VK_UP},
    {L"RIGHT", VK_RIGHT}, {L"RIGHTARROW", VK_RIGHT},
    {L"DOWN", VK_DOWN}, {L"DOWNARROW", VK_DOWN}, {L"SELECT", VK_SELECT},
    {L"PRINT", VK_PRINT}, {L"EXECUTE", VK_EXECUTE},
    {L"PRINTSCREEN", VK_SNAPSHOT}, {L"SNAPSHOT", VK_SNAPSHOT}, {L"PRTSC", VK_SNAPSHOT},
    {L"INSERT", VK_INSERT}, {L"INS", VK_INSERT},
    {L"DELETE", VK_DELETE}, {L"DEL", VK_DELETE}, {L"HELP", VK_HELP},
    {L"LWIN", VK_LWIN}, {L"LEFT_WINDOWS", VK_LWIN},
    {L"RWIN", VK_RWIN}, {L"RIGHT_WINDOWS", VK_RWIN},
    {L"APPS", VK_APPS}, {L"CONTEXT_MENU", VK_APPS},
    {L"SLEEP", VK_SLEEP}, {L"NUMLOCK", VK_NUMLOCK},
    {L"SCROLLLOCK", VK_SCROLL}, {L"SCROLL", VK_SCROLL},

    // Modifier keys. Generic forms match either side; side-specific forms do not.
    {L"SHIFT", VK_SHIFT}, {L"CTRL", VK_CONTROL}, {L"CONTROL", VK_CONTROL},
    {L"ALT", VK_MENU}, {L"MENU", VK_MENU},
    {L"LSHIFT", VK_LSHIFT}, {L"RSHIFT", VK_RSHIFT},
    {L"LCTRL", VK_LCONTROL}, {L"LCONTROL", VK_LCONTROL},
    {L"RCTRL", VK_RCONTROL}, {L"RCONTROL", VK_RCONTROL},
    {L"LALT", VK_LMENU}, {L"RALT", VK_RMENU},

    // Numpad operators.
    {L"MULTIPLY", VK_MULTIPLY}, {L"NUMPAD_MULTIPLY", VK_MULTIPLY}, {L"KP_MULTIPLY", VK_MULTIPLY},
    {L"ADD", VK_ADD}, {L"NUMPAD_ADD", VK_ADD}, {L"KP_ADD", VK_ADD},
    {L"SEPARATOR", VK_SEPARATOR}, {L"NUMPAD_SEPARATOR", VK_SEPARATOR}, {L"KP_SEPARATOR", VK_SEPARATOR},
    {L"SUBTRACT", VK_SUBTRACT}, {L"NUMPAD_SUBTRACT", VK_SUBTRACT}, {L"KP_SUBTRACT", VK_SUBTRACT},
    {L"DECIMAL", VK_DECIMAL}, {L"NUMPAD_DECIMAL", VK_DECIMAL}, {L"KP_DECIMAL", VK_DECIMAL},
    {L"DIVIDE", VK_DIVIDE}, {L"NUMPAD_DIVIDE", VK_DIVIDE}, {L"KP_DIVIDE", VK_DIVIDE},

    // Browser, volume, media, and application-launch keys.
    {L"BROWSER_BACK", VK_BROWSER_BACK}, {L"BROWSER_FORWARD", VK_BROWSER_FORWARD},
    {L"BROWSER_REFRESH", VK_BROWSER_REFRESH}, {L"BROWSER_STOP", VK_BROWSER_STOP},
    {L"BROWSER_SEARCH", VK_BROWSER_SEARCH},
    {L"BROWSER_FAVORITES", VK_BROWSER_FAVORITES},
    {L"BROWSER_HOME", VK_BROWSER_HOME}, {L"VOLUME_MUTE", VK_VOLUME_MUTE}, {L"MUTE", VK_VOLUME_MUTE},
    {L"VOLUME_DOWN", VK_VOLUME_DOWN}, {L"VOLUME_UP", VK_VOLUME_UP},
    {L"MEDIA_NEXT", VK_MEDIA_NEXT_TRACK}, {L"MEDIA_NEXT_TRACK", VK_MEDIA_NEXT_TRACK},
    {L"MEDIA_PREV", VK_MEDIA_PREV_TRACK}, {L"MEDIA_PREV_TRACK", VK_MEDIA_PREV_TRACK},
    {L"MEDIA_PREVIOUS_TRACK", VK_MEDIA_PREV_TRACK},
    {L"MEDIA_STOP", VK_MEDIA_STOP}, {L"MEDIA_PLAY_PAUSE", VK_MEDIA_PLAY_PAUSE},
    {L"PLAY_PAUSE", VK_MEDIA_PLAY_PAUSE},
    {L"LAUNCH_MAIL", VK_LAUNCH_MAIL}, {L"LAUNCH_MEDIA_SELECT", VK_LAUNCH_MEDIA_SELECT},
    {L"MEDIA_SELECT", VK_LAUNCH_MEDIA_SELECT},
    {L"LAUNCH_APP1", VK_LAUNCH_APP1}, {L"LAUNCH_APP2", VK_LAUNCH_APP2},

    // Main-keyboard punctuation/OEM keys.
    {L"OEM_1", VK_OEM_1}, {L"SEMICOLON", VK_OEM_1}, {L"US_SEMICOLON", VK_OEM_1},
    {L"OEM_PLUS", VK_OEM_PLUS}, {L"EQUALS", VK_OEM_PLUS}, {L"US_EQUALS", VK_OEM_PLUS},
    {L"OEM_COMMA", VK_OEM_COMMA}, {L"COMMA", VK_OEM_COMMA}, {L"US_COMMA", VK_OEM_COMMA},
    {L"OEM_MINUS", VK_OEM_MINUS}, {L"MINUS", VK_OEM_MINUS}, {L"US_MINUS", VK_OEM_MINUS},
    {L"OEM_PERIOD", VK_OEM_PERIOD}, {L"PERIOD", VK_OEM_PERIOD}, {L"US_PERIOD", VK_OEM_PERIOD},
    {L"OEM_2", VK_OEM_2}, {L"SLASH", VK_OEM_2}, {L"US_SLASH", VK_OEM_2},
    {L"OEM_3", VK_OEM_3}, {L"GRAVE", VK_OEM_3}, {L"TILDE", VK_OEM_3},
    {L"US_GRAVE", VK_OEM_3}, {L"US_TILDE", VK_OEM_3},
    {L"OEM_4", VK_OEM_4}, {L"LBRACKET", VK_OEM_4}, {L"US_LBRACKET", VK_OEM_4},
    {L"OEM_5", VK_OEM_5}, {L"BACKSLASH", VK_OEM_5}, {L"US_BACKSLASH", VK_OEM_5},
    {L"OEM_6", VK_OEM_6}, {L"RBRACKET", VK_OEM_6}, {L"US_RBRACKET", VK_OEM_6},
    {L"OEM_7", VK_OEM_7}, {L"APOSTROPHE", VK_OEM_7}, {L"QUOTE", VK_OEM_7},
    {L"US_APOSTROPHE", VK_OEM_7}, {L"US_QUOTE", VK_OEM_7},
    {L"OEM_8", VK_OEM_8}, {L"OEM_102", VK_OEM_102},

    // IME and less-common standard keyboard keys.
    {L"KANA", VK_KANA}, {L"HANGUL", VK_HANGUL}, {L"JUNJA", VK_JUNJA},
    {L"FINAL", VK_FINAL}, {L"HANJA", VK_HANJA}, {L"KANJI", VK_KANJI},
    {L"CONVERT", VK_CONVERT}, {L"NONCONVERT", VK_NONCONVERT},
    {L"ACCEPT", VK_ACCEPT}, {L"MODECHANGE", VK_MODECHANGE},
    {L"PROCESSKEY", VK_PROCESSKEY}, {L"PACKET", VK_PACKET},
    {L"ATTN", VK_ATTN}, {L"CRSEL", VK_CRSEL}, {L"EXSEL", VK_EXSEL},
    {L"EREOF", VK_EREOF}, {L"PLAY", VK_PLAY}, {L"ZOOM", VK_ZOOM},
    {L"PA1", VK_PA1}, {L"OEM_CLEAR", VK_OEM_CLEAR},
};

}  // namespace detail

constexpr int ParseVirtualKey(const wchar_t* value, int fallback) noexcept {
    if (value == nullptr || value[0] == L'\0') {
        return fallback;
    }

    // Windows assigns ASCII-compatible virtual-key values to A-Z and 0-9.
    if (value[1] == L'\0') {
        const wchar_t single = detail::UpperAscii(value[0]);
        if ((single >= L'A' && single <= L'Z') ||
            (single >= L'0' && single <= L'9')) {
            return static_cast<int>(single);
        }
    }

    constexpr int kNotMatched = -1;
    const int rawVirtualKey = detail::ParseHexVirtualKey(value, kNotMatched);
    if (rawVirtualKey != kNotMatched) {
        return rawVirtualKey;
    }
    std::size_t vkPrefixLength{};
    if (detail::ConsumePrefix(value, L"VK_", vkPrefixLength) &&
        value[vkPrefixLength] != L'\0') {
        return ParseVirtualKey(value + vkPrefixLength, fallback);
    }

    int parsed = detail::ParseNumberedKey(value, L"F", 1, 24, VK_F1, kNotMatched);
    if (parsed != kNotMatched) {
        return parsed;
    }
    parsed = detail::ParseNumberedKey(
        value, L"NUMPAD", 0, 9, VK_NUMPAD0, kNotMatched);
    if (parsed != kNotMatched) {
        return parsed;
    }
    parsed = detail::ParseNumberedKey(value, L"NUM", 0, 9, VK_NUMPAD0, kNotMatched);
    if (parsed != kNotMatched) {
        return parsed;
    }
    parsed = detail::ParseNumberedKey(value, L"KP", 0, 9, VK_NUMPAD0, kNotMatched);
    if (parsed != kNotMatched) {
        return parsed;
    }

    for (const detail::NamedKey& named : detail::kNamedKeys) {
        if (detail::EqualToken(value, named.name)) {
            return named.virtualKey;
        }
    }
    return fallback;
}

// Compile-time checks exercise the production parser across every key category.
static_assert(ParseVirtualKey(L"A", -1) == 'A');
static_assert(ParseVirtualKey(L"9", -1) == '9');
static_assert(ParseVirtualKey(L"f24", -1) == VK_F24);
static_assert(ParseVirtualKey(L"NUMPAD9", -1) == VK_NUMPAD9);
static_assert(ParseVirtualKey(L"kp0", -1) == VK_NUMPAD0);
static_assert(ParseVirtualKey(L"HOME", -1) == VK_HOME);
static_assert(ParseVirtualKey(L"rctrl", -1) == VK_RCONTROL);
static_assert(ParseVirtualKey(L"OEM_1", -1) == VK_OEM_1);
static_assert(ParseVirtualKey(L"mEdIa_PlAy_PaUsE", -1) == VK_MEDIA_PLAY_PAUSE);
static_assert(ParseVirtualKey(L"XBUTTON1", -1) == VK_XBUTTON1);
static_assert(ParseVirtualKey(L"VK_HOME", -1) == VK_HOME);
static_assert(ParseVirtualKey(L"VK:0xE2", -1) == 0xE2);
static_assert(ParseVirtualKey(L"VK_0x69", -1) == VK_NUMPAD9);
static_assert(ParseVirtualKey(L"F25", VK_F10) == VK_F10);
static_assert(ParseVirtualKey(L"NUMPAD10", VK_F10) == VK_F10);
static_assert(ParseVirtualKey(L"VK_0x00", VK_F10) == VK_F10);
static_assert(ParseVirtualKey(L"NUMPAD9junk", VK_F10) == VK_F10);

}  // namespace marker_teleport::key_config
