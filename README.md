# EqualizerAPO64-Hotkey

A fork of [EqualizerAPO](https://sourceforge.net/projects/equalizerapo/) (double precision, 64-bit edition)
with **global hotkey support for the Editor** added back to the UI.

## What is this?

This project extends the [equalizerAPO64](https://github.com/TheFireKahuna/equalizerAPO64) build (a double-precision,
AVX-optimized 64-bit port of EqualizerAPO) with a feature that the original Editor lacked:

- **Per-filter global hotkeys** — assign a system-wide keyboard shortcut (e.g. `Alt+X`) to any filter
  to toggle its enabled state from any application, even when the Editor is in the background.

The feature was originally present in older EqualizerAPO releases but was removed at some point. This
project restores it, with a clean implementation that works correctly under Qt 6.

## Features (added by this fork)

- ✅ **Global hotkey registration** via Win32 `RegisterHotKey` API
- ✅ **Per-filter shortcuts** stored in the Windows registry (`HKCU\Software\EqualizerAPO\Hotkeys`)
- ✅ **Works while Editor is in the background** (uses a dedicated worker thread + `GetMessage` loop,
  bypassing Qt 6's event loop which does not forward `WM_HOTKEY` to native event filters)
- ✅ **Persistent settings** — hotkeys survive Editor restart
- ✅ **Conflict detection** — refuses to register a hotkey that is already in use

All upstream features are preserved:
- 64-bit double precision audio processing
- AVX2 / AVX-512 optimizations
- Compatible with the original EqualizerAPO filter configuration format

## Differences from upstream equalizerAPO64

| File | Change |
|------|--------|
| `Editor/helpers/HotkeyManager.h` | **New** — hotkey manager + worker thread |
| `Editor/helpers/HotkeyManager.cpp` | **New** — registration, dispatch, persistence |
| `Editor/FilterTable.h` / `.cpp` | Owns the `HotkeyManager`, connects signals |
| `Editor/guis/CommentFilterGUI.*` | UI to assign a hotkey to a filter |
| `Editor/Editor.pro` | Lists the new files in the Qt project |
| `version.h` | Version bump to 1.4.2 (this fork) |
| `Editor/build/build.bat` | Adds `nmake clean` step to avoid stale `.obj` caching |

## Building

### Prerequisites

- Windows 10/11 x64
- Visual Studio 2022 with C++ workload
- Qt 6.10+ (`msvc2022_64` kit)
- All upstream dependencies — see [upstream README](https://github.com/TheFireKahuna/equalizerAPO64) for the
  full list and the `LIBSNDFILE_INCLUDE`, `FFTW_INCLUDE`, `MUPARSERX_INCLUDE`, `TCLAP_ROOT` env vars.

### Steps

```cmd
cd Editor\build
build.bat
```

The installer will be produced under `Setup\EqualizerAPO-x64-1.4.2-Hotkey.exe`.

## Usage

1. Install the released installer (or build from source).
2. Open the Editor.
3. Right-click any filter in the filter chain → **Set Hotkey** (or use the UI control).
4. Press the desired key combination (e.g. `Alt+X`).
5. With the Editor running in the background, press your hotkey — the filter toggles.

Hotkeys are stored in the registry under
`HKCU\Software\EqualizerAPO\Hotkeys\<filterId>\keySequence`.

## Implementation notes

The hotkey feature has a non-obvious implementation detail: under Qt 6, the main event loop does not use
the standard Win32 `GetMessage` API for message retrieval, so `WM_HOTKEY` messages posted by
`RegisterHotKey` to the main thread's message queue are never dispatched to user code. This fork
solves it by running a dedicated `HotkeyWorker` thread with a classic `GetMessage` loop. See
`HotkeyManager.cpp` for the full details and the comments explaining the workaround.

A second pitfall — and one that cost several iterations during development — is that the `HotkeyManager`
must **not** be a Qt child of `FilterTable`, because `FilterTable::updateGuis()` calls `deleteLater()`
on all child objects. If `HotkeyManager` is a child, it gets destroyed and its worker thread is
killed mid-flight, breaking all subsequent hotkey registrations. The fix is to pass `nullptr` as the
parent in the `HotkeyManager` constructor.

## License

This project is licensed under the **GNU General Public License v2** (or later), the same as the
upstream EqualizerAPO. See [License.txt](License.txt) for the full text.

### Attribution

- Original EqualizerAPO — © Jonas Thedering and contributors. https://sourceforge.net/projects/equalizerapo/
- Double-precision 64-bit port — https://github.com/TheFireKahuna/equalizerAPO64
- Hotkey restoration and Qt 6 compatibility work — this fork.

## Contributing

Issues and pull requests are welcome. For major changes please open an issue first to discuss.

## Project status

Stable. The hotkey feature has been tested with Alt+X-style combinations on Windows 10/11 x64 with
Qt 6.10. The implementation deliberately avoids hooking into Qt's event filter chain because Qt 6
no longer reliably forwards `WM_HOTKEY` through that path.
