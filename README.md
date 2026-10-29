# 日序 · DaySequence

一款适用于 Windows 的简约每日计划软件：提前安排目标，记录每天的完成情况，用图表回顾整个月。

[下载 Windows 版](https://github.com/walkeroliver/DaySequence/releases/latest) · [提交问题](https://github.com/walkeroliver/DaySequence/issues) · [GPL-3.0 许可证](LICENSE)

## 界面

![每日计划](docs/images/daily.png)

![月度回顾](docs/images/monthly.png)

截图使用示例数据。

## 功能

- 每日计划：按日期添加、编辑、完成目标，支持优先级、排序与搜索。
- 月度回顾：每日完成数量柱状图、整月完成比例环形图、统计与 CSV 导出。
- 主题：中式、欧式、美式，以及苔绿、海盐蓝、雾紫、暖杏、夜色。
- 可选天气主题：根据指定城市的天气自动换色，支持缓存及手动刷新。
- 本地记录：自动保存、全部计划备份、旧版数据导入。
- 桌面体验：系统托盘、单实例唤回、窗口位置记忆和小窗口滚动。

## 下载与运行

1. 在 [Releases](https://github.com/walkeroliver/DaySequence/releases) 下载 `DaySequence-2.1.0-windows-x64.zip`。
2. 完整解压 ZIP，双击文件夹中的 `DaySequence.exe`。
3. 请保留随附的 DLL 和 plugins 文件夹；不需要另行安装 Qt。

目标平台为 Windows x64。本次发布已在 Windows 11 验证，其他 Windows 版本尚未单独验证。程序尚未进行代码签名。

右上角图标打开外观、天气、数据与备份设置。关闭窗口会留在系统托盘；左下角电源按钮或托盘菜单完全退出。

## 数据与隐私

计划保存在本机，通常位于 `%LOCALAPPDATA%/DaySequence/DaySequence/plans-v2.json`，实际位置可在右上角设置 → 数据与备份中查看。卸载或替换程序文件夹不会主动删除计划。

天气默认关闭；开启后向 Open-Meteo 发送填写的城市及查询得到的城市坐标，不发送任务内容。程序不包含账号注册或云同步。

首次运行且新数据文件不存在时，可导入旧版 `%LOCALAPPDATA%/Daymark/plans.json`。旧文件保留，两个版本之间不会实时同步。

## 从源码构建

需要 Qt 6.8+（Core、Widgets、Network，测试另需 Test）、CMake 3.19+、Ninja 和匹配的 C++17 编译器。发布包使用 Qt 6.8.3 / MinGW 13.1 x64。

使用 Qt Creator 打开 `CMakeLists.txt`，选择匹配的 Qt 套件即可。也可在 Qt 构建环境中运行：

```powershell
cmake -S . -B build/release -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="你的 Qt 套件目录"
cmake --build build/release --parallel 4
ctest --test-dir build/release --output-on-failure
cmake --install build/release --prefix dist/DaySequence
```

打包便携版与源码包：

```powershell
./scripts/package.ps1 -QtRoot "你的 Qt 套件目录" -CompilerRoot "对应的 MinGW 目录" -IncludeSource
```

脚本先编译和测试，再生成 `artifacts/` 下的 ZIP 和 SHA256SUMS.txt。源码包取自当前 Git HEAD，请先提交要发布的源码。

## 项目结构

- `planner.*`：数据校验、保存、导入、排序、月度统计及导出。
- `mainwindow.*`：主界面、导航与目标操作。
- `tasklist.*`：列表模型与按需绘制。
- `visuals.*` / `appearance.cpp` / `icons.*`：日历、图表、主题与图标。
- `preferences.cpp` / `weather*.cpp`：设置与天气。
- `tests.cpp` / `uitests.cpp`：数据及界面测试。
- `scripts/package.ps1` / `.github/workflows/windows.yml`：本地打包与 GitHub 自动构建。

开发预览：`DaySequence.exe --preview <输出目录>`。预览使用隔离的示例数据和设置，不读取真实计划。

## 开源与依赖

Copyright (C) 2026 WalkerOliver。项目源码采用 **GPL-3.0-only**，详见 [LICENSE](LICENSE)。Qt 与 MinGW 运行库保留其原有许可证，详见 [第三方说明](THIRD_PARTY_NOTICES.md) 和 `licenses/`。

天气服务：[Open-Meteo](https://open-meteo.com/)。

发布维护说明见 [docs/RELEASING.md](docs/RELEASING.md)。
