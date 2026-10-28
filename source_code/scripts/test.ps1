$ErrorActionPreference = 'Stop'
& (Join-Path $PSScriptRoot 'build.ps1') -Tests
$sourceRoot = Split-Path $PSScriptRoot -Parent
$testOutput = @(& (Join-Path $sourceRoot 'build/parking_tests.exe'))
$testExitCode = $LASTEXITCODE
$testOutput | Set-Content -LiteralPath (Join-Path $sourceRoot 'build/test_results.txt') -Encoding UTF8
$testOutput | ForEach-Object { Write-Host $_ }
if ($testExitCode -ne 0) { throw 'Kiem thu that bai.' }
& (Join-Path $PSScriptRoot 'build.ps1')
& (Join-Path $sourceRoot 'tests/cli_smoke.ps1')
