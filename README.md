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

These checks exercise native boundaries with an in-memory test setup. Live session
testing is still required for scene transitions and third-party mods.
