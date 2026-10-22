param(
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Release",
    [switch]$SkipTests,
    [switch]$Package
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
Set-Location $root

function Invoke-Checked([string]$Program, [string[]]$Arguments) {
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "'$Program $($Arguments -join ' ')' failed with exit code $LASTEXITCODE."
    }
}

$vcpkgRoot = if ($env:VCPKG_ROOT -and (Test-Path "$env:VCPKG_ROOT/vcpkg.exe")) {
    $env:VCPKG_ROOT
} elseif (Test-Path "C:/vcpkg/vcpkg.exe") {
    "C:/vcpkg"
} elseif (Test-Path "C:/tools/vcpkg/vcpkg.exe") {
    "C:/tools/vcpkg"
} else {
    throw "vcpkg was not found. Install Visual Studio 2022 with Desktop development with C++, install vcpkg, and set VCPKG_ROOT to its folder."
}

$vcpkg = Join-Path $vcpkgRoot "vcpkg.exe"
$toolchain = Join-Path $vcpkgRoot "scripts/buildsystems/vcpkg.cmake"
Invoke-Checked $vcpkg @("install", "lua:x64-windows")

$buildDir = Join-Path $root "build-windows"
$tests = if ($SkipTests) { "OFF" } else { "ON" }
Invoke-Checked "cmake" @(
    "-S", ".", "-B", $buildDir,
    "-G", "Visual Studio 17 2022", "-A", "x64",
    "-DCMAKE_TOOLCHAIN_FILE=$toolchain",
    "-DVCPKG_TARGET_TRIPLET=x64-windows",
    "-DNUC_ENABLE_SANITIZERS=OFF",
    "-DNUC_BUILD_TESTS=$tests"
)

Invoke-Checked "cmake" @("--build", $buildDir, "--config", $Configuration, "--parallel", "2")
if (-not $SkipTests) {
    Invoke-Checked "ctest" @("--test-dir", $buildDir, "-C", $Configuration, "--output-on-failure")
}

if ($Package) {
    $stage = Join-Path $root "package-windows"
    if (Test-Path $stage) { Remove-Item $stage -Recurse -Force }
    New-Item -ItemType Directory -Force $stage | Out-Null
    Copy-Item (Join-Path $buildDir "$Configuration/nuc_app.exe") $stage
    Copy-Item "config.toml" $stage
    Copy-Item "data" $stage -Recurse
    Copy-Item "mods" $stage -Recurse
    Copy-Item "scripts" $stage -Recurse
    Get-ChildItem (Join-Path $vcpkgRoot "installed/x64-windows/bin") -Filter "lua*.dll" -ErrorAction SilentlyContinue | Copy-Item -Destination $stage
    @"
Nuc-lVuc-l portable build
=========================

Run nuc_app.exe from this folder. Keep data, mods, scripts and config.toml
beside the executable. The current renderer is a clear-screen prototype;
this package is for checking the engine/runtime systems, not a finished game.
"@ | Set-Content (Join-Path $stage "RUN-ME.txt")
    $archive = Join-Path $root "Nuc-lVuc-l-Windows-x64.zip"
    if (Test-Path $archive) { Remove-Item $archive -Force }
    Compress-Archive -Path (Join-Path $stage "*") -DestinationPath $archive -Force
    Write-Host "Package created: $archive"
}

Write-Host "Windows $Configuration build completed."
