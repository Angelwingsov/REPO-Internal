# R.E.P.O Internal

[![C++17](https://img.shields.io/badge/Standard-C%2B%2B17-blue.svg)](https://en.wikipedia.org/wiki/C%2B%2B17)
[![Platform](https://img.shields.io/badge/Platform-Windows%20x64-lightgrey.svg)](https://microsoft.com)
[![DirectX](https://img.shields.io/badge/Renderer-DirectX%2011-green.svg)](https://learn.microsoft.com/en-us/windows/win32/direct3d11/atoc-dx-graphics-direct3d-11)
[![GUI](https://img.shields.io/badge/GUI-Dear%20ImGui-orange.svg)](https://github.com/ocornut/imgui)

An internal cheat for **R.E.P.O** (DirectX 11, x64).

![Preview](preview/preview.png)

## Hotkeys

| Key | Action |
| :--- | :--- |
| `Delete` / `Insert` | Toggle Menu |
| `F` | Third Person |
| `V` | Speedhack |
| `G` | God Mode |
| `N` | NoClip |

## Configs

Open the disk-icon tab, enter a profile name under the list and click **Create**.
Select a profile to **Save**, **Load**, or **Delete** it; deletion requires the
**Confirm delete** button. **Refresh** rereads the list, and the mouse wheel scrolls
long lists. **Reset settings** restores defaults without deleting profiles.

Profiles are stored as `.ini` files in `%LOCALAPPDATA%\REPO-Internal\configs`.
They include feature settings, colors and hotkeys. Player addresses, session status,
current activation state and unload requests are excluded. Invalid files leave
current settings unchanged.

## Player ESP

ESP labels use the menu's Verdana 12 font. Player boxes follow active MeshRenderer
and SkinnedMeshRenderer parts under the avatar mesh on each game update, adapting
to crouching, crawling and tumbling. Animated head position corrects conservative
skinned-mesh bounds and supplies a fallback when mesh bounds are unavailable.
There is no forced 8-pixel minimum for player box height.

## Camera removals

In **Removals**, enable **Enabled** and select effects in the **Environment**,
**Post-processing**, and **Camera & screen** multi-select groups. Select **Shake / bob**
in **Camera & screen** to suppress impact shake and walking/running camera bob. Original updates and footstep audio continue;
turning the option or Removals off restores normal camera behavior on the next update.
The option is included in saved profiles. **Sway** controls noise separately. Multiple effects can be selected in each group.

## Runtime and diagnostics

Gameplay updates, scene collection, and queued menu actions run on `GameDirector.Update`.
The DX11 overlay draws native snapshots; it does not access managed scene objects.
Required field offsets come from the loaded game's metadata. Missing fields disable
gameplay initialization instead of falling back to guessed memory addresses.

Release x64 builds include a matching PDB in the output directory. Keep it with the
DLL when diagnosing a crash. The game's working directory contains `repo_cheat.log`
and, after an unhandled exception, `repo_cheat_crash.log` and `repo_cheat_crash.dmp`.
The crash report records the build, module, and relative fault address.

## Checks

Run native memory and concurrency tests from a Windows checkout with
Visual Studio C++ tools installed:

```bat
tests\run_native_tests.cmd
```

The native suite also checks profile file operations, validation and player ESP bounds.

Run `tests\run_ui_tests.cmd` to render and check the original GUI without injecting
the game. Previews are written to `build/ui-preview/`.

These checks exercise native boundaries with an in-memory test setup. Live session
testing is still required for scene transitions and third-party mods.
