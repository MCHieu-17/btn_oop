param([switch]$Tests)
$ErrorActionPreference = 'Stop'
$sourceRoot = Split-Path $PSScriptRoot -Parent
$buildDirectory = Join-Path $sourceRoot 'build'
New-Item -ItemType Directory -Force -Path $buildDirectory | Out-Null
$compiler = Get-Command g++ -ErrorAction SilentlyContinue
if (-not $compiler) { throw 'Khong tim thay g++. Hay cai MSYS2 UCRT64 va them thu muc bin vao PATH.' }
$sourceFiles = @(Get-ChildItem -LiteralPath (Join-Path $sourceRoot 'src') -Filter '*.cpp' -Recurse)
if ($Tests) {
    $sourceFiles = @($sourceFiles | Where-Object { $_.Name -ne 'main.cpp' -and $_.Directory.Name -ne 'ui' })
    $sourceFiles += Get-Item -LiteralPath (Join-Path $sourceRoot 'tests/parking_tests.cpp')
    $outputFile = Join-Path $buildDirectory 'parking_tests.exe'
} else {
    $outputFile = Join-Path $buildDirectory 'parking.exe'
}
$compilerArguments = @('-std=c++17', '-Wall', '-Wextra', '-Wpedantic', '-O2', '-finput-charset=UTF-8', '-fexec-charset=UTF-8', '-I', 'include')
# Dùng đường dẫn tương đối để MinGW xử lý được thư mục cha có dấu tiếng Việt.
$compilerArguments += @($sourceFiles | ForEach-Object { $_.FullName.Substring($sourceRoot.Length + 1) })
$relativeOutput = if ($Tests) { 'build/parking_tests.exe' } else { 'build/parking.exe' }
$compilerArguments += @('-o', $relativeOutput)
Push-Location -LiteralPath $sourceRoot
try {
    & $compiler.Source @compilerArguments
    if ($LASTEXITCODE -ne 0) { throw 'Bien dich that bai.' }
} finally {
    Pop-Location
}
Write-Host "Build thanh cong: $outputFile"
