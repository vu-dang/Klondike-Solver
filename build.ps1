#Requires -Version 5.1
<#
.SYNOPSIS
    Build the Klondike solver executable(s) with MSVC (cl.exe).
.DESCRIPTION
    make/g++ are not on PATH on this machine, so this script loads the Visual Studio
    C++ environment via vcvars64.bat and invokes cl.exe. Run it from anywhere; it
    builds into the repo root next to this script. obj\ and *.exe are gitignored.
.PARAMETER Target
    Which executable to build: SimpleSolver (default), KlondikeSolver, or all.
.PARAMETER DebugBuild
    Produce an unoptimized debug build (/Od /Zi) instead of the default /O2 release.
.EXAMPLE
    .\build.ps1
.EXAMPLE
    .\build.ps1 all
.EXAMPLE
    .\build.ps1 KlondikeSolver -DebugBuild
#>
param(
    [ValidateSet('SimpleSolver', 'KlondikeSolver', 'all')]
    [string]$Target = 'SimpleSolver',
    [switch]$DebugBuild
)

$ErrorActionPreference = 'Stop'
$root = $PSScriptRoot

# Shared translation units used by both front ends (everything except the main() files).
$shared = 'Card.cpp Move.cpp Pile.cpp Random.cpp Solitaire.cpp'

function Find-VcVars {
    # Prefer vswhere (handles any edition/version); fall back to known install paths.
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere) {
        $install = & $vswhere -latest -products * `
            -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
            -property installationPath
        if ($install) {
            $candidate = Join-Path $install 'VC\Auxiliary\Build\vcvars64.bat'
            if (Test-Path $candidate) { return $candidate }
        }
    }
    $guesses = @(
        'C:\Program Files\Microsoft Visual Studio\18\Community\VC\Auxiliary\Build\vcvars64.bat'
        'C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat'
        'C:\Program Files\Microsoft Visual Studio\2022\Professional\VC\Auxiliary\Build\vcvars64.bat'
        'C:\Program Files\Microsoft Visual Studio\2022\Enterprise\VC\Auxiliary\Build\vcvars64.bat'
    )
    foreach ($g in $guesses) { if (Test-Path $g) { return $g } }
    throw 'Could not find vcvars64.bat. Edit build.ps1 to point at your VC++ install.'
}

# Map each requested target to its entry-point source file.
$targets = @{}
if ($Target -eq 'all') {
    $targets['SimpleSolver']   = 'SimpleSolver.cpp'
    $targets['KlondikeSolver'] = 'KlondikeSolver.cpp'
} else {
    $targets[$Target] = "$Target.cpp"
}

$flags = if ($DebugBuild) {
    '/EHsc /std:c++17 /Od /Zi /D_CRT_SECURE_NO_WARNINGS'
} else {
    '/EHsc /std:c++17 /O2 /D_CRT_SECURE_NO_WARNINGS'
}

$vcvars = Find-VcVars

Push-Location $root
try {
    foreach ($name in $targets.Keys) {
        $entry  = $targets[$name]
        $objdir = "obj\$name\"
        New-Item -ItemType Directory -Force $objdir | Out-Null
        Write-Host "Building $name ($entry)..." -ForegroundColor Cyan
        # vcvars64.bat and cl must run in the same cmd session; >nul hides the VS banner.
        $cl = "cl /nologo $flags /Fe:$name.exe /Fo:$objdir $entry $shared"
        cmd /c "`"$vcvars`" >nul && $cl"
        if ($LASTEXITCODE -ne 0) { throw "$name build failed (exit code $LASTEXITCODE)." }
        Write-Host "  -> $name.exe" -ForegroundColor Green
    }
} finally {
    Pop-Location
}
