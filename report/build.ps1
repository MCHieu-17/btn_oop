param([ValidateSet('Auto', 'Tectonic', 'XeLaTeX')][string]$Engine = 'Auto')
$ErrorActionPreference = 'Stop'
$reportRoot = $PSScriptRoot
$portableEngine = Join-Path $reportRoot '.tools/tectonic.exe'
$xelatexCommand = Get-Command xelatex -ErrorAction SilentlyContinue
if ($Engine -eq 'Auto') {
    $Engine = if (Test-Path -LiteralPath $portableEngine) { 'Tectonic' } elseif ($xelatexCommand) { 'XeLaTeX' } else { 'Tectonic' }
}
if ($Engine -eq 'Tectonic' -and -not (Test-Path -LiteralPath $portableEngine)) {
    $systemEngine = Get-Command tectonic -ErrorAction SilentlyContinue
    if ($systemEngine) {
        $portableEngine = $systemEngine.Source
    } else {
        $toolsDirectory = Join-Path $reportRoot '.tools'
        New-Item -ItemType Directory -Force -Path $toolsDirectory | Out-Null
        $archive = Join-Path $toolsDirectory 'tectonic-0.17.0.zip'
        $downloadUrl = 'https://github.com/tectonic-typesetting/tectonic/releases/download/tectonic%400.17.0/tectonic-0.17.0-x86_64-pc-windows-msvc.zip'
        Write-Host 'Downloading Tectonic 0.17.0 from the official GitHub release...'
        Invoke-WebRequest -Uri $downloadUrl -OutFile $archive -UseBasicParsing
        Expand-Archive -LiteralPath $archive -DestinationPath $toolsDirectory -Force
    }
}
New-Item -ItemType Directory -Force -Path (Join-Path $reportRoot 'build') | Out-Null
$previousFontConfig = $env:FONTCONFIG_FILE
if ($Engine -eq 'Tectonic' -and -not $env:FONTCONFIG_FILE) {
    $env:FONTCONFIG_FILE = 'fonts.conf'
}
Push-Location -LiteralPath $reportRoot
try {
    if ($Engine -eq 'XeLaTeX') {
        if (-not $xelatexCommand) { throw 'XeLaTeX was not found in PATH.' }
        for ($pass = 1; $pass -le 3; $pass++) {
            & $xelatexCommand.Source '-interaction=nonstopmode' '-halt-on-error' '-output-directory=build' 'parking_report.tex'
            if ($LASTEXITCODE -ne 0) { throw "XeLaTeX failed on pass $pass." }
        }
    } else {
        & $portableEngine '--keep-logs' '--keep-intermediates' '--outdir' 'build' 'parking_report.tex'
        if ($LASTEXITCODE -ne 0) { throw 'Tectonic compilation failed.' }
    }
    Copy-Item -LiteralPath 'build/parking_report.pdf' -Destination 'parking_report.pdf' -Force
    Write-Host "PDF ready: $(Join-Path $reportRoot 'parking_report.pdf')"
} finally {
    Pop-Location
    if ($null -eq $previousFontConfig) {
        Remove-Item -LiteralPath 'Env:FONTCONFIG_FILE' -ErrorAction SilentlyContinue
    } else {
        $env:FONTCONFIG_FILE = $previousFontConfig
    }
}
