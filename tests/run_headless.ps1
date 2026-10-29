param(
    [ValidateRange(10000, 100000000)]
    [int]$IterationsPerOperator = 100000
)

$ErrorActionPreference = 'Stop'
$project = Split-Path -Parent $PSScriptRoot
$testSource = Join-Path $PSScriptRoot 'test_calculator.c'
$testExecutable = Join-Path $PSScriptRoot 'calculator-headless-tests.exe'

Push-Location $project
try {
    gcc -O2 -std=c11 -Wall -Wextra -Wpedantic $testSource -lm -o $testExecutable
    if ($LASTEXITCODE -ne 0) {
        throw 'Headless test compilation failed'
    }

    & $testExecutable $IterationsPerOperator
    if ($LASTEXITCODE -ne 0) {
        throw 'Headless stress tests failed'
    }
}
finally {
    Pop-Location
}
