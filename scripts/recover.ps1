param(
    [Parameter()]
    [ValidatePattern('^v\d+\.\d+\.\d+$')]
    [string]$Version = 'v1.2.8',

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
    windres -J rc -O coff calculator.rc calculator-resource.o
    if ($LASTEXITCODE -ne 0) { throw 'Icon resource compilation failed' }

    # Link compiler runtimes statically, mark the image DEP-compatible, and
    # omit linker symbols. DbgHelp is loaded from System32 at startup.
    $buildArguments = @(
        '-std=c11', '-Wall', '-Wextra', '-Wpedantic', '-mwindows', '-static',
        '-Wl,--nxcompat', '-Wl,--strip-all',
        'calculator.c', 'calculator-resource.o', '-o', 'calculator-gui.exe'
    )
    & gcc @buildArguments
    if ($LASTEXITCODE -ne 0) { throw 'Calculator rebuild failed' }

    Remove-Item calculator-resource.o -ErrorAction SilentlyContinue
}
finally {
    Pop-Location
}

Write-Host "Recovered and rebuilt $Version in $fullDestination"
Write-Host "Launch with: `"$fullDestination\calculator-gui.exe`""
