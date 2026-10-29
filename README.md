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
- 本地记录：默认便携存储，也可自选保存位置；自动保存并支持全部计划备份。
- 桌面体验：系统托盘、单实例唤回、窗口位置记忆和小窗口滚动。
- 统一下拉菜单：主题配色、清晰选中标识、方向键/回车/Esc 操作，收起时滚轮不会误改选项。

## 下载与运行

1. 在 [Releases](https://github.com/walkeroliver/DaySequence/releases) 下载 DaySequence-2.2.0-windows-x64.zip。
2. 完整解压 ZIP，双击文件夹中的 DaySequence.exe。
3. 保留随附的 DLL 和 plugins 文件夹；不需要另行安装 Qt。

目标平台为 Windows x64。本次发布在 Windows 11 验证，其他 Windows 版本尚未单独验证。程序尚未进行代码签名。

右上角图标打开外观、天气、数据与备份设置。关闭窗口会留在系统托盘；左下角电源按钮或托盘菜单完全退出。

## 数据与隐私

默认使用便携模式，任务和偏好保存在程序旁的 data 文件夹：

- tasks.json：计划与完成状态。
- settings.ini：主题、排序、天气及窗口偏好。

在设置的「数据与备份」页点击「更改位置…」，可以选择其他保存文件夹。点击「保存设置」并确认后，程序复制当前任务与偏好，重新启动后使用新位置。原文件保留；目标文件夹已有不同数据或正被另一个日序使用时停止迁移，避免覆盖。

自定义位置记录在程序旁的 storage.json，只含保存路径。选择自定义位置后，备份、移动电脑或升级时请同时保留这个文件和它指向的数据文件夹；仅复制程序不能带走外部数据。若恢复为程序旁的 data，请再次通过设置选择该目录。程序目录需要可写权限以保存位置配置。

完全退出后，可备份并删除**当前保存位置**的 tasks.json 清空任务；删除该位置的 settings.ini 可重置偏好。删除 storage.json 只恢复默认保存位置，不删除外部数据。发布 ZIP 和源码包不含个人数据或保存位置配置。

天气默认关闭；开启后向 Open-Meteo 发送填写的城市及查询得到的城市坐标，不发送任务内容。程序不包含账号注册或云同步。

已删除 Daymark 旧数据读取与导入功能，不再读取或写入旧版注册表偏好。如需保留此前便携版的当前格式记录，请完全退出后，将 data/plans-v2.json 手动改名为 data/tasks.json；程序不会自动迁移旧文件。

## 从源码构建

需要 Qt 6.8+（Core、Widgets、Network，测试另需 Test）、CMake 3.19+、Ninja 和匹配的 C++17 编译器。发布包使用 Qt 6.8.3 / MinGW 13.1 x64。

使用 Qt Creator 打开 CMakeLists.txt，选择匹配的 Qt 套件即可。也可在 Qt 构建环境中运行：

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

脚本先编译和测试，再生成 artifacts/ 下的 ZIP 和 SHA256SUMS.txt。源码包取自当前 Git HEAD，请先提交要发布的源码。

## 项目结构

```text
src/
  app/          启动、单实例、数据位置解析与重启
  core/         任务、排序、月度统计与文件校验
  services/     保存位置迁移、天气数据处理
  ui/           窗口、主题、设置与天气交互
    widgets/    日历、图表、任务列表、统一下拉选择控件
    components/ 图标和公共界面组件
resources/      SVG、Windows ICO 与版本资源
tests/          任务、界面、存储迁移测试
scripts/        构建、测试、部署与打包
```

架构边界与存储流程见 [架构说明](docs/ARCHITECTURE.md)。Windows EXE 内嵌多尺寸图标和产品版本信息。

开发预览：DaySequence.exe --preview <输出目录>。使用隔离的示例数据和设置，不读取真实计划。

## 开源与依赖

Copyright (C) 2026 WalkerOliver。项目源码采用 **GPL-3.0-only**，详见 [LICENSE](LICENSE)。Qt 与 MinGW 运行库保留其原有许可证，详见 [第三方说明](THIRD_PARTY_NOTICES.md) 和 licenses/。

天气服务：[Open-Meteo](https://open-meteo.com/)。发布维护说明见 [docs/RELEASING.md](docs/RELEASING.md)。