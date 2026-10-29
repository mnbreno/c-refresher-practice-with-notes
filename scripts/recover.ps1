param(
    [Parameter(Mandatory = $true)]
    [ValidatePattern('^v\d+\.\d+\.\d+$')]
    [string]$Version,

    [Parameter(Mandatory = $true)]
    [string]$Destination
)

$ErrorActionPreference = 'Stop'
$repository = 'https://github.com/mnbreno/c-refresher-practice-with-notes.git'
$fullDestination = [System.IO.Path]::GetFullPath($Destination)

if (Test-Path $fullDestination) {
    throw "Destination already exists; choose a new folder to avoid overwriting data: $fullDestination"
}

git clone --branch $Version --single-branch $repository $fullDestination
if ($LASTEXITCODE -ne 0) {
    throw "Recovery clone failed for $Version"
}

Push-Location $fullDestination
try {
    windres calculator.rc calculator-resource.o
    if ($LASTEXITCODE -ne 0) { throw 'Icon resource compilation failed' }

    gcc -std=c11 -Wall -Wextra -Wpedantic -mwindows calculator.c calculator-resource.o -ldbghelp -o calculator-gui.exe
    if ($LASTEXITCODE -ne 0) { throw 'Calculator rebuild failed' }

    Remove-Item calculator-resource.o -ErrorAction SilentlyContinue
}
finally {
    Pop-Location
}

Write-Host "Recovered and rebuilt $Version in $fullDestination"
Write-Host "Launch with: `"$fullDestination\calculator-gui.exe`""
