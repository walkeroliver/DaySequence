[CmdletBinding()]
param(
    [string]$QtRoot = $env:QT_ROOT_DIR,
    [string]$CompilerRoot,
    [string]$CMakePath = 'cmake',
    [string]$BuildDirectory = 'build/package',
    [switch]$IncludeSource
)
$ErrorActionPreference = 'Stop'
$project = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
Set-Location -LiteralPath $project
if (!$QtRoot) { throw '请传入 -QtRoot，或在 Qt 构建环境中运行。' }
$QtRoot = [IO.Path]::GetFullPath($QtRoot)
if ($CompilerRoot) { $env:PATH = (Join-Path $CompilerRoot 'bin') + ';' + $env:PATH }
$env:PATH = (Join-Path $QtRoot 'bin') + ';' + $env:PATH
$version = [regex]::Match([IO.File]::ReadAllText((Join-Path $project 'CMakeLists.txt')), 'project\(DaySequence VERSION ([0-9.]+)').Groups[1].Value
if (!$version) { throw '无法读取项目版本号' }
if ($IncludeSource) {
    $changes = @(git status --porcelain)
    if ($LASTEXITCODE -ne 0 -or $changes.Count -gt 0) { throw '请先提交要发布的源码，避免源码包和程序不一致' }
}
$build = [IO.Path]::GetFullPath((Join-Path $project $BuildDirectory))
if (!$build.StartsWith($project + '\', [StringComparison]::OrdinalIgnoreCase)) { throw '构建目录必须位于项目内' }
$output = Join-Path $project 'artifacts'
$stage = [IO.Path]::GetFullPath((Join-Path $build 'portable/DaySequence'))
if (!$stage.StartsWith($build + '\', [StringComparison]::OrdinalIgnoreCase)) { throw '打包目录必须位于构建目录内' }
if (Test-Path -LiteralPath $stage) {
    if ((Test-Path -LiteralPath (Join-Path $stage 'data')) -or (Test-Path -LiteralPath (Join-Path $stage 'storage.json'))) { throw '暂存目录含个人数据或保存位置设置，请使用其他构建目录打包' }
    if (((Get-Item -LiteralPath $stage -Force).Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) { throw '打包目录不能是目录链接' }
    Remove-Item -LiteralPath $stage -Recurse -Force
}
New-Item -ItemType Directory -Path $output -Force | Out-Null
& $CMakePath -S $project -B $build -G Ninja "-DCMAKE_PREFIX_PATH=$QtRoot" -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
if ($LASTEXITCODE -ne 0) { throw 'CMake 配置失败' }
& $CMakePath --build $build --parallel 4
if ($LASTEXITCODE -ne 0) { throw '编译失败' }
$ctest = Join-Path (Split-Path (Get-Command $CMakePath).Source) 'ctest.exe'
& $ctest --test-dir $build --output-on-failure
if ($LASTEXITCODE -ne 0) { throw '测试未通过，停止打包' }
& $CMakePath --install $build --prefix $stage
if ($LASTEXITCODE -ne 0) { throw '运行库部署失败' }
$exe = Join-Path $stage 'DaySequence.exe'
if ((Get-Item -LiteralPath $exe).VersionInfo.FileVersion -ne $version) { throw 'EXE 版本信息与项目版本不一致' }
if ((Test-Path -LiteralPath (Join-Path $stage 'data')) -or (Test-Path -LiteralPath (Join-Path $stage 'storage.json'))) { throw '发布目录出现个人数据，停止打包' }
$binary = Join-Path $output "DaySequence-$version-windows-x64.zip"
Compress-Archive -LiteralPath $stage -DestinationPath $binary -Force -CompressionLevel Optimal
$assets = @($binary)
if ($IncludeSource) {
    $source = Join-Path $output "DaySequence-$version-source.zip"
    git archive --format=zip "--prefix=DaySequence-$version/" "--output=$source" HEAD
    if ($LASTEXITCODE -ne 0) { throw '源码归档失败' }
    $assets += $source
}
$checksums = @(foreach ($file in $assets) {
    $hash = Get-FileHash -LiteralPath $file -Algorithm SHA256
    $hash.Hash.ToLowerInvariant() + '  ' + [IO.Path]::GetFileName($file)
})
[IO.File]::WriteAllLines((Join-Path $output 'SHA256SUMS.txt'), $checksums, (New-Object Text.UTF8Encoding($false)))
Get-Item -LiteralPath ($assets + @((Join-Path $output 'SHA256SUMS.txt'))) | Select-Object Name, Length
