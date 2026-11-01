# 项目架构

项目使用 Qt Widgets / C++17，按职责组织源码，并以两个静态库复用实现。

| 层 | 目录 / 构建目标 | 职责 |
| --- | --- | --- |
| 启动 | src/app / DaySequence | 应用版本、默认与自定义目录、单实例锁、重启 |
| 数据 | src/core / daysequence_data | 任务模型、校验、原子保存、排序、统计、导出 |
| 服务 | src/services / daysequence_data | StorageManager 保存位置迁移、天气数据解析 |
| 界面 | src/ui / daysequence_ui | 主窗口、偏好、主题、天气请求与显示 |
| 控件 | src/ui/widgets | 日历、图表、任务列表、ChoiceBox 下拉菜单 |
| 公共组件 | src/ui/components | 矢量图标、通用布局辅助 |
| 资源 | resources | SVG、ICO、Windows RC 产品信息 |
| 验证 | tests | 核心逻辑、界面操作、迁移和错误处理 |

核心数据层不依赖窗口。界面通过 Planner 操作任务，通过 StorageManager 请求位置迁移；启动与重新启动由入口统一负责。主题颜色通过 Theme 传给自绘控件，ChoiceBox 复用 QComboBox 的选项模型与信号，统一绘制和交互。

## 存储与位置切换

1. 启动读取程序旁的 storage.json；没有配置时使用程序旁的 data。配置损坏时显示错误，避免悄悄使用其他目录。
2. 在所选数据目录取得 app.lock，再读取 tasks.json 与 settings.ini。不使用注册表，不自动读取旧版文件。
3. 用户在设置中选择目录并确认，先保存当前任务、偏好和窗口位置。
4. StorageManager 验证目录和目标数据，锁定目标目录，原子复制任务与偏好；不同内容不覆盖，已占用目录不迁移。
5. 成功后原子写入位置配置。失败时撤回本次新建的数据文件，源文件始终保留。
6. 主窗口发出重启信号；入口释放旧目录锁与单实例服务，再启动新进程读取新目录。

自定义路径保留为绝对路径；程序旁的默认 data 保存为相对路径。复制整个程序目录只对默认便携存储自动成立，外部数据需要单独备份。

## 发布

CMake 是版本号唯一来源，同时生成应用版本和 Windows 资源信息。app.svg 用于运行时图标，app.ico 为 EXE 内嵌的多尺寸 Windows 图标。修改品牌图形时应同步两者。

scripts/package.ps1 从干净的部署目录收集 Qt 与 MinGW 运行库，运行四组测试，检查版本与个人数据排除规则，再生成程序包、当前提交的源码包及 SHA256 校验。使用 -IncludeSource 时要求工作区已提交，保证源码与程序一致。
## 退出行为

MainWindow 的各退出入口统一调用 requestExit，保存偏好、取消网络请求、隐藏托盘后发送 exitRequested。应用入口将信号排队连接到 quit，确保窗口已经隐藏时也结束事件循环；单实例服务与目录锁随入口栈对象销毁释放。托盘可见时窗口关闭只保存偏好并隐藏窗口；托盘不可用时关闭会退出。托盘菜单和电源按钮直接进入统一退出流程，不再依赖 close 或 lastWindowClosed。lifecycle_tests 使用独立子进程验证隐藏和可见窗口的退出，并检查已保存任务与偏好。

## 专注时钟

Countdown 位于 src/core，通过 QElapsedTimer 计算实际经过的时间，QTimer 只负责刷新，不累减固定秒数。状态为准备、运行、暂停、完成；完成信号每轮只发出一次。FocusClock 位于 src/ui/widgets，负责时长输入、预设、环形绘制和交互，界面切页不会销毁计时器。主窗口接收完成信号并负责系统提示音和托盘提醒。仅时长与提示音偏好写入 settings.ini，运行中的计时会话不跨程序重启保留。

## 2.4 完成铃声

FocusAlarm（src/services）使用 QMediaPlayer / QAudioOutput，提供单次、循环、试听上限、停止及错误回退。FocusClock 选择资源 URL 或经过校验的数据目录内音频路径，并保存铃声、循环和音量偏好；Countdown 继续只负责单调计时，不依赖音频设备。MainWindow 只显示托盘完成提醒，托盘停止入口委托给 FocusClock，不再调用 QApplication::beep。

用户音频保留文件名，通过 QSaveFile 写入 music/，设置更新失败保留原选择；StorageManager 复制 music/ 中的全部受支持音频，并保留源文件。内置音频编入 Qt 资源，发布不依赖开发机绝对路径。
