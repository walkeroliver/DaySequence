# 日序 v2.2.0

- 重新整理项目分层，便于维护和继续开发。
- Windows EXE 内嵌多尺寸图标、产品信息和版本号。
- 设置的「数据与备份」支持选择保存位置，复制任务与偏好并重启生效。保留原文件，遇到数据冲突或目录占用时停止迁移。
- 全部下拉菜单统一样式，增加清晰的选中标识、键盘操作，避免滚轮误改选项。

默认数据仍在程序旁的 data/tasks.json 和 data/settings.ini。自定义位置由程序旁的 storage.json 记录；移动电脑时请同时复制该配置和它指向的数据目录。不读取旧版数据或注册表，不自动导入记录。

下载 DaySequence-2.2.0-windows-x64.zip，完整解压到可写入的位置后运行 DaySequence.exe。
源码为 DaySequence-2.2.0-source.zip，SHA256SUMS.txt 提供校验值。源码采用 GPL-3.0-only。