# EqualizerAPO64-Hotkey

[English](README.md) | 简体中文

[EqualizerAPO](https://sourceforge.net/projects/equalizerapo/) 的一个分支（双精度、64 位版本），
在界面中恢复了**编辑器的全局热键支持**。

## 这是什么？

本项目在 [equalizerAPO64](https://github.com/TheFireKahuna/equalizerAPO64)（EqualizerAPO 的双精度、
AVX 优化 64 位移植版）的基础上，扩展了原版编辑器所缺少的功能：

- **按滤波器绑定全局热键** —— 为任意滤波器指定系统级快捷键（例如 `Alt+X`），
  即可在任意应用中切换其启用状态，即使编辑器在后台运行也有效。

该功能曾存在于旧版 EqualizerAPO 中，但在某个版本被移除。本项目将其恢复，
并以适配 Qt 6 的干净实现重新落地。

## 特性（本分支新增）

- ✅ 基于 Win32 `RegisterHotKey` API 的**全局热键注册**
- ✅ **按滤波器配置快捷键**，保存于 Windows 注册表（`HKCU\Software\EqualizerAPO\Hotkeys`）
- ✅ **编辑器在后台时依然有效**（使用独立工作线程 + `GetMessage` 消息循环，
  绕过 Qt 6 事件循环——它不会把 `WM_HOTKEY` 转发给原生事件过滤器）
- ✅ **设置持久化** —— 热键在编辑器重启后依然保留
- ✅ **冲突检测** —— 拒绝注册已被占用的热键

所有上游特性均完整保留：
- 64 位双精度音频处理
- AVX2 / AVX-512 优化
- 兼容原版 EqualizerAPO 滤波器配置格式

## 与上游 equalizerAPO64 的差异

| 文件 | 变更 |
|------|------|
| `Editor/helpers/HotkeyManager.h` | **新增** —— 热键管理器 + 工作线程 |
| `Editor/helpers/HotkeyManager.cpp` | **新增** —— 注册、分发、持久化 |
| `Editor/FilterTable.h` / `.cpp` | 持有 `HotkeyManager`，连接信号 |
| `Editor/guis/CommentFilterGUI.*` | 为滤波器指定热键的 UI |
| `Editor/Editor.pro` | 在 Qt 工程中列出新增文件 |
| `version.h` | 版本号升至 1.4.2（本分支） |
| `Editor/build/build.bat` | 增加 `nmake clean` 步骤，避免过期的 `.obj` 缓存 |

## 构建

### 前置要求

- Windows 10/11 x64
- Visual Studio 2022（含 C++ 工作负载）
- Qt 6.10+（`msvc2022_64` 套件）
- 所有上游依赖 —— 完整列表及 `LIBSNDFILE_INCLUDE`、`FFTW_INCLUDE`、`MUPARSERX_INCLUDE`、
  `TCLAP_ROOT` 环境变量请见[上游 README](https://github.com/TheFireKahuna/equalizerAPO64)。

### 步骤

```cmd
cd Editor\build
build.bat
```

安装包将生成在 `Setup\EqualizerAPO-x64-1.4.2-Hotkey.exe`。

## 使用

1. 安装发布版安装包（或从源码构建）。
2. 打开编辑器。
3. 在滤波器链中右键任意滤波器 → **Set Hotkey**（或使用界面控件）。
4. 按下所需的组合键（例如 `Alt+X`）。
5. 让编辑器在后台运行，按下热键——滤波器即被切换。

热键保存在注册表
`HKCU\Software\EqualizerAPO\Hotkeys\<filterId>\keySequence` 下。

## 实现说明

该热键功能有一个不那么显然的实现细节：在 Qt 6 下，主事件循环并不使用标准的 Win32
`GetMessage` API 取消息，因此 `RegisterHotKey` 投递到主线程消息队列的 `WM_HOTKEY`
消息永远不会分发给用户代码。本分支通过运行独立的 `HotkeyWorker` 线程、使用经典
`GetMessage` 循环来解决。完整细节及解释该绕行方案的注释见 `HotkeyManager.cpp`。

第二个坑——开发过程中耗费了数轮迭代——是 `HotkeyManager` **不能**作为 `FilterTable`
的 Qt 子对象，因为 `FilterTable::updateGuis()` 会对所有子对象调用 `deleteLater()`。
若 `HotkeyManager` 是子对象，它会被销毁，其工作线程会在运行中被杀死，
从而破坏后续所有热键注册。解决办法是在 `HotkeyManager` 构造函数中把父对象传为 `nullptr`。

## 许可证

本项目采用 **GNU 通用公共许可证 v2**（或更高版本），与上游 EqualizerAPO 相同。
完整文本见 [License.txt](License.txt)。

### 致谢

- 原始 EqualizerAPO —— © Jonas Thedering 及贡献者。https://sourceforge.net/projects/equalizerapo/
- 双精度 64 位移植 —— https://github.com/TheFireKahuna/equalizerAPO64
- 热键恢复与 Qt 6 兼容性工作 —— 本分支。

## 参与贡献

欢迎提交 Issue 和 Pull Request。重大改动请先开 Issue 讨论。

## 项目状态

稳定。热键功能已在 Windows 10/11 x64、Qt 6.10 环境下使用 Alt+X 类组合键测试通过。
实现有意避开 Qt 的事件过滤器链，因为 Qt 6 不再可靠地通过该路径转发 `WM_HOTKEY`。
