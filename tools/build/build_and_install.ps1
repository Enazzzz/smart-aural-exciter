# build_and_install.ps1
# ---------------------
# One-shot Windows build + install for the SmartExciter VST3.
# Run from the repo root:
#
#     powershell -ExecutionPolicy Bypass -File tools\build\build_and_install.ps1
#
# What it does:
#   1. Configures the CMake project under .\build (Visual Studio 2022, x64).
#   2. Builds Release.
#   3. Copies the resulting "Smart Exciter.vst3" bundle into the system
#      VST3 path so REAPER picks it up on next scan.

$ErrorActionPreference = "Stop"

$repoRoot   = Resolve-Path "$PSScriptRoot\..\.."
$buildDir   = Join-Path $repoRoot "build"
$vst3Source = Join-Path $buildDir "SmartExciter_artefacts\Release\VST3\Smart Exciter.vst3"
$vst3Dest   = Join-Path $env:CommonProgramFiles "VST3"

Write-Host "Repo root   : $repoRoot"
Write-Host "Build dir   : $buildDir"
Write-Host "VST3 dest   : $vst3Dest"

if (-not (Test-Path $buildDir)) {
	Write-Host "Configuring CMake project..."
	cmake -S $repoRoot -B $buildDir -G "Visual Studio 17 2022" -A x64
}

Write-Host "Building Release..."
cmake --build $buildDir --config Release --target SmartExciter

if (-not (Test-Path $vst3Source)) {
	throw "Expected build artefact not found: $vst3Source"
}

if (-not (Test-Path $vst3Dest)) {
	Write-Host "Creating VST3 destination..."
	New-Item -ItemType Directory -Path $vst3Dest | Out-Null
}

Write-Host "Copying VST3 bundle to system path..."
$destBundle = Join-Path $vst3Dest "Smart Exciter.vst3"
if (Test-Path $destBundle) {
	Remove-Item -Recurse -Force $destBundle
}
Copy-Item -Recurse -Path $vst3Source -Destination $vst3Dest

Write-Host "Done. Restart REAPER or rescan plugins to pick up the new build."
