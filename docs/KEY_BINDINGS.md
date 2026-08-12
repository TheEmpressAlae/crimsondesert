# Key bindings

`General.ReloadKey` and `Teleport.Hotkey` use the same case-insensitive key-name parser. Unknown or malformed values retain that field's default (`F11` for reload and `F10` for teleport).

## Direct names

- Letters and top-row digits: `A`–`Z`, `0`–`9`
- Function keys: `F1`–`F24`
- Numpad digits: `NUMPAD0`–`NUMPAD9`; aliases `NUM0`–`NUM9` and `KP0`–`KP9`
- Editing: `BACKSPACE`, `TAB`, `CLEAR`, `ENTER`, `ESCAPE`, `SPACE`, `INSERT`, `DELETE`, `HELP`
- Navigation: `PAGEUP`, `PAGEDOWN`, `HOME`, `END`, `LEFT`, `UP`, `RIGHT`, `DOWN`
- Modifiers: `SHIFT`, `CTRL`, `ALT`, `LSHIFT`, `RSHIFT`, `LCTRL`, `RCTRL`, `LALT`, `RALT`
- System and locks: `CANCEL`, `PAUSE`, `CAPSLOCK`, `LWIN`, `RWIN`, `APPS`, `SLEEP`, `NUMLOCK`, `SCROLLLOCK`, `PRINTSCREEN`
- Numpad operators: `NUMPAD_MULTIPLY`, `NUMPAD_ADD`, `NUMPAD_SEPARATOR`, `NUMPAD_SUBTRACT`, `NUMPAD_DECIMAL`, `NUMPAD_DIVIDE`; replace `NUMPAD_` with `KP_` for the short aliases
- Mouse: `MOUSE_LEFT`, `MOUSE_RIGHT`, `MOUSE_MIDDLE`, `MOUSE_X1`, `MOUSE_X2`; aliases `MOUSE1`–`MOUSE5`, `LBUTTON`, `RBUTTON`, `MBUTTON`, `XBUTTON1`, `XBUTTON2`
- Browser: `BROWSER_BACK`, `BROWSER_FORWARD`, `BROWSER_REFRESH`, `BROWSER_STOP`, `BROWSER_SEARCH`, `BROWSER_FAVORITES`, `BROWSER_HOME`
- Media and volume: `VOLUME_MUTE`, `VOLUME_DOWN`, `VOLUME_UP`, `MEDIA_NEXT_TRACK`, `MEDIA_PREV_TRACK`, `MEDIA_STOP`, `MEDIA_PLAY_PAUSE`
- Launch: `LAUNCH_MAIL`, `LAUNCH_MEDIA_SELECT`, `LAUNCH_APP1`, `LAUNCH_APP2`

Common short aliases such as `ESC`, `SPACEBAR`, `PGUP`, `PGDN`, `INS`, `DEL`, `PRTSC`, `CONTROL`, `MUTE`, `MEDIA_NEXT`, `MEDIA_PREV`, and `PLAY_PAUSE` are also accepted. A standard name may optionally use its Windows-style `VK_` prefix, such as `VK_HOME` or `VK_F10`.

## Punctuation and keyboard layouts

Layout-stable physical-key names are `OEM_1`, `OEM_PLUS`, `OEM_COMMA`, `OEM_MINUS`, `OEM_PERIOD`, `OEM_2`, `OEM_3`, `OEM_4`, `OEM_5`, `OEM_6`, `OEM_7`, `OEM_8`, and `OEM_102`.

Friendly names `SEMICOLON`, `EQUALS`, `COMMA`, `MINUS`, `PERIOD`, `SLASH`, `GRAVE`, `TILDE`, `LBRACKET`, `BACKSLASH`, `RBRACKET`, `APOSTROPHE`, and `QUOTE` follow a US keyboard layout. Their explicit `US_` forms are also accepted.

## Raw Windows virtual-key values

For uncommon hardware or a key without a friendly name, use exactly two hexadecimal digits:

```ini
[Teleport]
Hotkey=VK:0x69
```

`VK_0x69` is an accepted alias. Values `0x01` through `0xFE` are allowed; `0x00`, `0xFF`, missing digits, extra digits, signs, and trailing text are rejected. Raw values may represent reserved or hardware-specific keys, so confirm them in game.

## Notes

- `NUMPAD9` is distinct from top-row `9`.
- Num Lock should be enabled for numpad digit bindings.
- Generic `SHIFT`, `CTRL`, and `ALT` accept either side. Use the `L`/`R` variants when side matters.
- Both Enter keys report the same standard Windows virtual key.
