$ErrorActionPreference = 'Stop'
$sourceRoot = Split-Path $PSScriptRoot -Parent
$buildDirectory = Join-Path $sourceRoot 'build'
$runName = 'cli_smoke_' + [Guid]::NewGuid().ToString('N')
$runDirectory = Join-Path $buildDirectory $runName
New-Item -ItemType Directory -Force -Path $runDirectory | Out-Null

function Invoke-Parking([string[]]$InputLines, [string]$RelativeDirectory, [bool]$Demo = $true, [int]$ExpectedExit = 0) {
    $startInfo = New-Object System.Diagnostics.ProcessStartInfo
    $startInfo.FileName = Join-Path $buildDirectory 'parking.exe'
    $startInfo.WorkingDirectory = $sourceRoot
    $startInfo.Arguments = '--data-dir ' + $RelativeDirectory
    if ($Demo) { $startInfo.Arguments += ' --demo' }
    $startInfo.UseShellExecute = $false
    $startInfo.CreateNoWindow = $true
    $startInfo.RedirectStandardInput = $true
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true
    $startInfo.StandardOutputEncoding = [System.Text.Encoding]::UTF8
    $startInfo.StandardErrorEncoding = [System.Text.Encoding]::UTF8
    $process = New-Object System.Diagnostics.Process
    $process.StartInfo = $startInfo
    $null = $process.Start()
    $outputTask = $process.StandardOutput.ReadToEndAsync()
    $errorTask = $process.StandardError.ReadToEndAsync()
    $process.StandardInput.Write(($InputLines -join "`n") + "`n")
    $process.StandardInput.Close()
    $process.WaitForExit()
    $outputText = $outputTask.Result + $errorTask.Result
    $exitCode = $process.ExitCode
    $process.Dispose()
    if ($exitCode -ne $ExpectedExit) { throw "CLI exit $exitCode, expected $ExpectedExit. $outputText" }
    return $outputText
}

function Assert-True([bool]$Condition, [string]$Message) {
    if (-not $Condition) { throw $Message }
}

try {
    $relativeMain = "build/$runName/main"
    $mainInput = @('3', 'abc', '1', 'C001', '29A1-12345', '1', '2026-10-26 08:00',
        '2', 'C001', '2026-10-26 17:00', 'y', 'n', '3',
        '2', 'C001', '2026-10-26 19:00', 'n', '3',
        '2', 'C001', '2026-10-26 19:00', 'y', 'y',
        '1', 'C001', '29M-99999', '2', '2026-10-26 19:00',
        '2', 'C001', '2026-10-26 21:00', 'y', 'y',
        '1', 'C002', '29B1-88888', '1', '2026-10-26 18:00',
        '2', 'C002', '2026-10-27 08:00', 'y', '12000', 'Phi qua dem theo xac nhan', 'y',
        '6', '2026-10-26', '6', '2026-10-27',
        '1', 'C003', '29A5-77777', '1', '2026-10-27 08:00', '0')
    $mainOutput = Invoke-Parking $mainInput $relativeMain
    $mainFile = Join-Path $runDirectory 'main/parking_data.txt'
    $lines = @(Get-Content -LiteralPath $mainFile -Encoding UTF8)
    Assert-True ($lines.Count -eq 5) 'Expected exactly four sessions after cancellations and card reuse.'
    Assert-True ($lines[0] -eq 'PARKING_DATA_V1 3 5') 'Wrong capacity or next id.'
    Assert-True (($lines | Where-Object { $_ -match ' 7000 0 ""$' }).Count -eq 1) 'Mixed fee not saved.'
    Assert-True (($lines | Where-Object { $_ -match ' 5000 0 ""$' }).Count -eq 1) 'Night fee not saved.'
    Assert-True (($lines | Where-Object { $_ -match ' 12000 1 ' }).Count -eq 1) 'Overnight fee not saved.'
    Assert-True (($lines | Where-Object { $_ -match '"-" 0 0 ""$' }).Count -eq 1) 'Expected one active session.'
    foreach ($amount in @('3.000', '7.000', '5.000', '12.000')) {
        Assert-True ($mainOutput.Contains($amount)) "Missing amount $amount in console output."
    }
    $mainOutput | Set-Content -LiteralPath (Join-Path $buildDirectory 'cli_demo_output.txt') -Encoding UTF8
    Write-Host 'PASS C01_menu_cancel_reuse_overnight_and_statistics'

    $beforeRestart = [System.IO.File]::ReadAllText($mainFile)
    $restartOutput = Invoke-Parking @('3', '5', '0') $relativeMain
    Assert-True ($restartOutput.Contains('29A577777')) 'Active plate not restored in console.'
    Assert-True ($restartOutput.Contains('7.000')) 'Paid history not restored in console.'
    Assert-True ([System.IO.File]::ReadAllText($mainFile) -eq $beforeRestart) 'Read-only menu changed data.'
    Write-Host 'PASS C02_restart_preserves_active_and_history'

    $normalOutput = Invoke-Parking @('1', '1', 'N001', '29N-12345', '1', '3', '0') "build/$runName/normal" $false
    $normalFile = Join-Path $runDirectory 'normal/parking_data.txt'
    $normalData = [System.IO.File]::ReadAllText($normalFile)
    Assert-True ($normalData.Contains((Get-Date -Format 'yyyy-MM-dd'))) 'Normal mode did not use system date.'
    Assert-True ($normalData.Contains('"-" 0 0 ""')) 'Normal entry not active.'
    Write-Host 'PASS C03_normal_mode_uses_system_clock'

    $badDirectory = Join-Path $runDirectory 'corrupt'
    New-Item -ItemType Directory -Force -Path $badDirectory | Out-Null
    $badFile = Join-Path $badDirectory 'parking_data.txt'
    [System.IO.File]::WriteAllText($badFile, 'BROKEN DATA')
    $null = Invoke-Parking @() "build/$runName/corrupt" $true 1
    Assert-True ([System.IO.File]::ReadAllText($badFile) -eq 'BROKEN DATA') 'Corrupt file overwritten.'
    Write-Host 'PASS C04_corrupt_startup_preserves_file'
    'RESULT 4 CLI scenarios passed' | Set-Content -LiteralPath (Join-Path $buildDirectory 'cli_results.txt') -Encoding UTF8
} finally {
    # Chỉ xóa thư mục kiểm thử tự tạo sau khi xác minh nó nằm trong build.
    $resolvedRun = [System.IO.Path]::GetFullPath($runDirectory)
    $allowedPrefix = [System.IO.Path]::GetFullPath($buildDirectory) + [System.IO.Path]::DirectorySeparatorChar
    if (-not $resolvedRun.StartsWith($allowedPrefix, [System.StringComparison]::OrdinalIgnoreCase)) {
        throw 'Refusing cleanup outside build directory.'
    }
    if (Test-Path -LiteralPath $resolvedRun) { Remove-Item -LiteralPath $resolvedRun -Recurse -Force }
}
