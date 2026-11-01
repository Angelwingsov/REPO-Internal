# Original Skeet GUI import review

Source: the user-supplied `il2cpp skeet/Gamesense_MiSide` folder. No executable, DLL, static library, build action or Lua script from that folder was executed.

## GUI source preservation

`src/ui/original/framework.*`, `renderer.*` and `color.h` come from the supplied GUI framework and renderer. The initial replacement skin was removed. Production uses the original drawing routines, foreground draw list, 660x560 default frame, 47 px sidebar icon font, eight icon-only tabs, six frame outlines, 2 px accent gradient, original group grid, scrolling, drag/resize behavior, checkboxes, buttons, sliders, dropdowns, keybind popup and color picker. `src/ui/menu.cpp` supplies current R.E.P.O. settings/actions to those primitives.

`tests/gui_source_contract.json` records normalized hashes from the supplied source for 34 drawing/interaction routines. `tests/gui_source_contract.py` verifies that their implementation still matches. Normalization only allows initialized local booleans, owned queued text, the key-code bounds guard, and `FontSize` -> `LegacySize` compatibility with the project's ImGui version.

## Compatibility and stability changes

- Includes and DX11 resource creation are connected to this project's existing backend. Deprecated D3DX binary dependencies are replaced by Windows WIC decoding of the same original PNG, at full 4096x4096 resolution. The original renderer still draws it at 1:1 scale and clips it to the menu; no rescaling or replacement texture.
- The original tab/icon, secondary icon and keybind fonts are embedded unchanged. Verdana regular/bold use the original 12 px sizes, keybind font uses 8 px, secondary icons 32 px. The keybind font uses the existing ImGui rasterizer instead of linking the supplied FreeType binary.
- GUI contexts/windows are value-initialized. Per-draw leaked allocations become value objects. Deferred text and item keys own their strings. An unused helper that wrote wide text into a narrow buffer is excluded. Empty selections and key indexes are checked.
- Input uses the project's menu key and high-bit key-down state, plus its existing mouse-wheel key support. Test builds can inject key states without operating the desktop. Render/input/game lifecycles remain synchronized by the existing project code.
- The source keybind popup's four modes map to the client's Hold/Toggle modes and the corresponding Always on/Off hotkey modes. Native tests cover all four.
- GUI contexts, fonts and texture references are cleaned up before overlay/device teardown.

## Original binary resources

| Resource | Bytes | SHA-256 |
| --- | ---: | --- |
| background.png | 424769 | eb2c1fc427fef48363b7f2d016178c3a31cd1a028daae67b1d5c8080849fdeb7 |
| tab_icons.ttf | 5192 | 41e0df0496aadc676801daac8e6b6e9586285816fbf5a4f3e231e6ad8b2d514a |
| legit_tab_icons.ttf | 47320 | b1e359b6e69f02427441d03a43d3221d9007aa9d29b7b4ae7bf7b5d80b099b66 |
| keybinds.ttf | 25600 | 0e71338a9ae45df2ede080567cfab831627bac9a83be601521ddbace4cbed11e |

## Source review and exclusions

Reviewed Main.cpp, Unity glue, menu/framework/renderer, configuration and Lua integration. Searched source text for network endpoints, common HTTP/download APIs, shell/process execution, registry persistence and credential-access APIs. No explicit theft or persistence was identified in this scoped review. This does not certify every file in the supplied folder.

The original Lua integration opens `jit`, `ffi`, `debug` and `package` libraries and executes script files. The GUI does not need this facility, so it is excluded. Original IL2CPP hooks, bundled executable/static libraries, Detours/MinHook binaries, project/build files and thread utilities are excluded. Existing project MinHook/Mono code remains in use.

## Verification

- `tests/run_ui_tests.cmd`: production GUI with stub game actions, rendered through DX11 WARP. Checks eight tabs, 660x560 geometry, font sizes/resources, checkbox/slider bindings, original keybind popup and color picker, clipping-stack balance, fade/close/reopen and resource cleanup. PNG previews: `build/ui-preview/`.
- `tests/gui_source_contract.py`: 34 source drawing/interaction routine comparisons.
- `tests/run_native_tests.cmd`: stability regressions and all four activation modes.
- Release/Debug x64 DLL builds. In-game injection/runtime validation remains separate.
