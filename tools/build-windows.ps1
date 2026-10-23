param(
    [ValidateSet("windows-debug", "windows-release")]
    [string]$Preset = "windows-release",
    [switch]$SkipTests
)

# Build on Windows through the CMake presets. No vcpkg: Lua is vendored by
# cmake/lua.cmake, everything else comes from FetchContent.
#
# The script finds vcvars64.bat, enters that environment and then runs plain
# cmake, so it works from any shell instead of only a Developer PowerShell.

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot

function Find-VcVars {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path $vswhere) {
        $install = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
        if ($install) {
            $candidate = Join-Path $install "VC\Auxiliary\Build\vcvars64.bat"
            if (Test-Path $candidate) { return $candidate }
        }
    }
    $probe = Get-ChildItem "${env:ProgramFiles}\Microsoft Visual Studio" -Filter "vcvars64.bat" -Recurse -ErrorAction SilentlyContinue |
        Select-Object -First 1
    if ($probe) { return $probe.FullName }
    throw "vcvars64.bat was not found. Install the Visual Studio C++ toolchain."
}

$vcvars = Find-VcVars
# <VS root>\VC\Auxiliary\Build\vcvars64.bat -> four levels up gives <VS root>.
$vsRoot = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $vcvars)))

# A plain shell has no cmake, ninja or git. FetchContent needs git for every
# GIT_REPOSITORY dependency, so expose all three before entering vcvars.
$extraPaths = @(
    (Join-Path $vsRoot "Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin"),
    (Join-Path $vsRoot "Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja"),
    (Join-Path $vsRoot "Common7\IDE\CommonExtensions\Microsoft\TeamFoundation\Team Explorer\Git\cmd")
) | Where-Object { Test-Path $_ }
$env:PATH = (($extraPaths + $env:PATH) -join ";")
foreach ($tool in @("cmake", "ninja", "git")) {
    if (-not (Get-Command $tool -ErrorAction SilentlyContinue)) {
        throw "'$tool' is not on PATH even after adding the Visual Studio tool directories."
    }
}

Write-Host "Environment: $vcvars"
Write-Host "Preset     : $Preset"

$steps = @(
    "cd /d `"$root`"",
    "chcp 65001 >nul",
    "call `"$vcvars`" >nul",
    "cmake --preset $Preset",
    "cmake --build --preset $Preset"
)
if (-not $SkipTests) {
    $steps += "ctest --preset $Preset"
}

$command = $steps -join " && "
& cmd.exe /c $command
if ($LASTEXITCODE -ne 0) {
    throw "Windows build failed with exit code $LASTEXITCODE."
}

# A non-English MSVC prints a localized "/showIncludes" line, and CMake records
# that prefix in rules.ninja in the console code page. Under the default OEM
# code page (866 on this machine) the recorded prefix turns into mojibake, Ninja
# stops matching it and every build reports "no work to do" after a header edit.
# Running the configure under UTF-8 keeps the prefix readable, so verify that the
# generated build graph really tracks headers instead of trusting it.
$buildDir = Join-Path $root "build/$Preset"
$ninja = Join-Path $vsRoot "Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja\ninja.exe"
$objectsRoot = Join-Path $buildDir "CMakeFiles\nuc_render.dir"
$sample = Get-ChildItem -LiteralPath $objectsRoot -Filter "*.obj" -Recurse -ErrorAction SilentlyContinue |
    Select-Object -First 1
if ($sample) {
    $relative = $sample.FullName.Substring($buildDir.Length + 1)
    $report = & $ninja -C $buildDir -t deps $relative 2>&1 | Select-Object -First 1
    if ($report -match "#deps 0") {
        throw "Header dependency tracking is broken for $relative (ninja reports 0 deps). Re-run the configure from a UTF-8 console: chcp 65001."
    }
    Write-Host "Dependency tracking: $report"
}

Write-Host "Windows build completed: build/$Preset"
