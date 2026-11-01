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
function Assert-CleanReleaseData([string]$Directory) {
    if (Test-Path -LiteralPath (Join-Path $Directory 'storage.json')) { throw '发布目录含个人保存位置配置，停止打包。' }
    $data = Join-Path $Directory 'data'
    if (!(Test-Path -LiteralPath $data)) { return }
    $expected = @('music/chime.wav', 'music/soft.wav', 'music/bell.wav')
    $entries = @(Get-ChildItem -LiteralPath $data -Recurse -Force)
    foreach ($entry in @((Get-Item -LiteralPath $data -Force)) + $entries) {
        if (($entry.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) { throw '发布数据目录不能包含链接。' }
    }
    foreach ($entry in $entries) {
        $relative = $entry.FullName.Substring($data.Length + 1).Replace('\', '/')
        if ($entry.PSIsContainer) {
            if ($relative -ne 'music') { throw '发布目录含额外数据文件夹，停止打包。' }
        } else {
            if ($relative -notin $expected) { throw '发布目录含用户数据或自选音乐，停止打包。' }
            $original = Join-Path $project ('resources/sounds/' + $entry.Name)
            if ((Get-FileHash -LiteralPath $entry.FullName -Algorithm SHA256).Hash -ne (Get-FileHash -LiteralPath $original -Algorithm SHA256).Hash) {
                throw '预设音乐已被替换，停止打包，避免带入个人音频。'
            }
        }
    }
}
if (Test-Path -LiteralPath $stage) {
    Assert-CleanReleaseData $stage
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
Assert-CleanReleaseData $stage
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
