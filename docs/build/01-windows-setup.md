# 01 - Windows Build Setup

This is the V1 build path on Windows x64.

## Prerequisites

Install once:

- **Visual Studio 2022 Community** (or Build Tools 2022) with the
  "Desktop development with C++" workload. We need MSVC v143+ and the
  Windows 10/11 SDK.
- **CMake 3.22+** (Visual Studio bundles a recent enough copy; the
  standalone CMake from kitware works too).
- **Git** (for `FetchContent` to clone JUCE on the first configure).
- A working **REAPER** install (for testing - not for building).

JUCE itself is pulled in at configure time via `FetchContent` in the root
`CMakeLists.txt` - no manual JUCE clone or environment variable needed.

## Configure and build

From the repository root, in a "Developer PowerShell for VS 2022" or any
shell where `cmake` and `cl.exe` are on PATH:

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```

The first configure downloads JUCE and may take a minute. Subsequent
configures are fast.

The built VST3 lands at:

```
build\SmartExciter_artefacts\Release\VST3\Smart Exciter.vst3\
```

## Install into REAPER

Copy the entire `Smart Exciter.vst3` bundle (it is a folder on Windows)
into your VST3 path. Default user-scope path:

```
%COMMONPROGRAMFILES%\VST3\
```

REAPER will pick it up on the next plugin scan. To force a rescan:
`Options -> Preferences -> Plug-ins -> VST -> Re-scan`.

A convenience PowerShell script that does build + install in one step is
in `tools\build\build_and_install.ps1`.

## Run the C++ tests

```powershell
cmake --build build --config Release --target test_determinism
cmake --build build --config Release --target test_safety_limiter
cmake --build build --config Release --target test_erb_layout

ctest --test-dir build -C Release --output-on-failure
```

These tests cover algorithmic invariants (determinism, ceiling enforcement,
ERB layout sanity). They do not replace the REAPER validation suite in
`docs/validation/04-validation-suite.md`.

## Common gotchas

- **JUCE pulled fresh on every configure?** That happens if
  `build/_deps/juce-src` is missing. Don't `git clean -dfx` between
  configures unless you want to redownload JUCE.
- **MSVC fatal error C1083 about a JUCE module header?** Reconfigure -
  this usually means `juce_generate_juce_header` hasn't run yet for a
  fresh build directory.
- **Plugin loads but produces silence in REAPER:** Confirm the buses are
  configured for stereo on the master. V1 explicitly rejects mono buses
  (see `PluginProcessor::isBusesLayoutSupported`).

## Why JUCE 8 (and not 7)?

JUCE 8 has cleaner CMake support, a working `juce::dsp::Oversampling`
without quirks at high oversampling factors, and ships with the
`juce::dsp::FFT` interface we use. Pinning is in `CMakeLists.txt` -
bumping deliberately, not automatically, keeps determinism intact across
contributors.
