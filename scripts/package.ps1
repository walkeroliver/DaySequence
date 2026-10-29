param(
    [string]$QtRoot = $env:QT_ROOT_DIR,
    [string]$CompilerRoot,
    [string]$CMakePath = 'cmake',
    [string]$BuildDirectory = 'build/package',
    [switch]$IncludeSource
)
$ErrorActionPreference='Stop'
$project=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
Set-Location -LiteralPath $project
if(!$QtRoot){throw '请传入 -QtRoot，或在 Qt 构建环境中运行。'}
$QtRoot=[IO.Path]::GetFullPath($QtRoot)
if($CompilerRoot){$env:PATH=(Join-Path $CompilerRoot 'bin')+';'+$env:PATH}
$env:PATH=(Join-Path $QtRoot 'bin')+';'+$env:PATH
$version=[regex]::Match([IO.File]::ReadAllText((Join-Path $project 'CMakeLists.txt')),'project\(DaySequence VERSION ([0-9.]+)').Groups[1].Value
if(!$version){throw '无法读取项目版本号'}
$build=[IO.Path]::GetFullPath((Join-Path $project $BuildDirectory))
if(!$build.StartsWith($project+'\',[StringComparison]::OrdinalIgnoreCase)){throw '构建目录必须位于项目内'}
$output=Join-Path $project 'artifacts'
$stage=Join-Path $project 'dist/DaySequence'
New-Item -ItemType Directory -Path $output -Force | Out-Null
& $CMakePath -S $project -B $build -G Ninja "-DCMAKE_PREFIX_PATH=$QtRoot" -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
if($LASTEXITCODE -ne 0){throw 'CMake 配置失败'}
& $CMakePath --build $build --parallel 4
if($LASTEXITCODE -ne 0){throw '编译失败'}
$ctest=Join-Path (Split-Path (Get-Command $CMakePath).Source) 'ctest.exe'
& $ctest --test-dir $build --output-on-failure
if($LASTEXITCODE -ne 0){throw '测试未通过，停止打包'}
& $CMakePath --install $build --prefix $stage
if($LASTEXITCODE -ne 0){throw '运行库部署失败'}
$binary=Join-Path $output "DaySequence-$version-windows-x64.zip"
Compress-Archive -LiteralPath $stage -DestinationPath $binary -Force -CompressionLevel Optimal
$assets=@($binary)
if($IncludeSource){
    $source=Join-Path $output "DaySequence-$version-source.zip"
    git archive --format=zip "--prefix=DaySequence-$version/" "--output=$source" HEAD
    if($LASTEXITCODE -ne 0){throw '源码归档失败：请先提交当前源码'}
    $assets+=$source
}
$checksums=@(foreach($file in $assets){$hash=Get-FileHash -LiteralPath $file -Algorithm SHA256;$hash.Hash.ToLowerInvariant()+'  '+[IO.Path]::GetFileName($file)})
[IO.File]::WriteAllLines((Join-Path $output 'SHA256SUMS.txt'),$checksums,(New-Object Text.UTF8Encoding($false)))
Get-ChildItem -LiteralPath $output -File | Select-Object Name,Length
