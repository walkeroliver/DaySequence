# 发布维护

版本号在 CMakeLists.txt 与 main.cpp 中保持一致，更新 RELEASE_NOTES.md 后提交源码。

1. 在 Qt 6.8.3 / MinGW x64 环境执行 scripts/package.ps1，或等待 GitHub Actions 构建。
2. 便携包、对应源码包和 SHA256SUMS.txt 位于 artifacts/。
3. 检查便携包解压启动、目标保存、主题切换及托盘操作。
4. 创建并推送版本标签，例如 git tag -a v2.1.1 -m "DaySequence v2.1.1"，然后 git push origin v2.1.1。
5. GitHub Actions 编译、测试、上传产物，建立 Draft Release。审核说明与附件后发布。

若相同版本已存在，自动流程不会覆盖其附件。修复已发布版本时使用新的版本号和标签。

发布包必须包含 Qt / MinGW 运行库及 LICENSE、NOTICE、THIRD_PARTY_NOTICES.md 和 licenses 文件夹。无需打包 build、.qtcreator、个人计划或任何凭据。

便携版禁止将个人 data 目录打进发布包；scripts/package.ps1 使用构建目录内的干净暂存目录部署并校验。测试实际运行请解压 ZIP 到另一目录，以免污染暂存目录。
