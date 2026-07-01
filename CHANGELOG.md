# Changelog

All notable changes to this project are documented here. The format is loosely based on
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [1.4.2] - 2026-07-01

First public release of this fork.

### Added
- **Global hotkeys for filters** — assign a system-wide shortcut to any filter in the Editor
  to toggle its enabled state from any application.
- `HotkeyManager` and `HotkeyWorker` classes (`Editor/helpers/HotkeyManager.{h,cpp}`).
- `CommentFilterGUI` UI control for picking a hotkey per filter.
- Persistent hotkey storage in `HKCU\Software\EqualizerAPO\Hotkeys`.
- Destructor and lifecycle logging (`%TEMP%\equalizerapo_hotkey.log`) to aid future debugging.

### Changed
- `HotkeyManager` is constructed with `QObject(nullptr)` to prevent `FilterTable::updateGuis()`
  from destroying it via `deleteLater()`.
- `Editor/build/build.bat` now calls `nmake clean` before building to avoid stale `.obj` caching.
- Version reset to 1.4.2 to align with the upstream EqualizerAPO base.

### Notes
- See `README.md` "Implementation notes" for the technical background on why a dedicated worker
  thread is required under Qt 6.
