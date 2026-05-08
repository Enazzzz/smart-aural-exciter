# Smart Exciter (working title)

A Windows VST3 smart exciter aimed at the master bus.

Goal: add clarity and detail to a finished mix without harshness or listener
fatigue, using a perceptual model that places harmonics only where the ear
actually expects them.

## Status

V1 implementation in progress. The DSP, UI, build system, tests, and
validation tooling all exist. The remaining work is the validation +
tuning loop in REAPER (`docs/build/02-validation-runbook.md`).

## Quick start

1. Read the build instructions: [docs/build/01-windows-setup.md](docs/build/01-windows-setup.md).
2. Build + install in one step:
	```powershell
	powershell -ExecutionPolicy Bypass -File tools\build\build_and_install.ps1
	```
3. Open REAPER. The plugin appears as `CoolMusic / Smart Exciter`.
4. To validate: follow [docs/build/02-validation-runbook.md](docs/build/02-validation-runbook.md).

## Repository layout

```
.cursor/                Cursor project configuration
src/
  plugin/               JUCE AudioProcessor + parameter schema + editor
  dsp/                  DSP modules (DspEngine, Oversampler, AnalysisBank,
                        HarmonicGenerator, HarshnessGuard, SafetyLimiter,
                        MeterTap, ErbBands, ActivitySnapshot)
  ui/                   Editor panels (SimplePanel, AdvancedPanel,
                        MeterView, ActivityView)
tests/
  dsp/                  Algorithmic invariants (determinism, ceiling, ERB)
tools/
  build/                Build + install PowerShell helper
  reaper/               ReaScript for batch-rendering the validation corpus
  measure/              Python loudness/peak/true-peak measurement script
docs/
  spec/                 Frozen V1 specification (scope, measurements,
                        DSP architecture)
  validation/           Reference corpus index, REAPER session notes,
                        per-slot scorecard template
  build/                Windows build instructions, validation runbook
```

The original V1 spec and implementation plan also live outside this repo at:

- `c:\Users\Zanea\.cursor\plans\smart-exciter-v1-spec_65b2f8c9.plan.md`
- `c:\Users\Zanea\.cursor\plans\smart-exciter-v1-implementation_be660005.plan.md`

The frozen `docs/spec/` files are the canonical, in-repo source of truth.

## Top-level decisions (locked)

- Format: VST3 only.
- Platform: Windows x64 only.
- Primary insertion point: stereo master bus.
- Default sonic identity: transparent.
- Quality bias: high quality first; latency budget allowed.
- Calibration target: electronic-first (Aphex-style detail/transients), rock-compatible.
- Validation audience: personal use first, occasional non-audiophile checks.
- License model: free in V1, paid/pro path later.
- Copy protection in V1: none.
- Framework: JUCE 8 (pulled via CMake FetchContent).
- Language: C++17.

## Known V1 limitations

These are deliberate trade-offs documented so they are not surprises in
the validation loop:

1. **Per-block FFT in HarmonicGenerator (no overlap-add).** Yields zero
   added latency from the harmonic stage but can produce minor cyclic
   convolution artifacts on highly transient material at block boundaries.
   Acceptable on master-bus content; revisit post-V1 if validation flags
   audible block-edge clicks.
2. **Polyphase IIR oversampling** rather than linear-phase FIR. Lowest
   CPU + smallest delay choice; trade-off is non-zero phase distortion
   that may show up in null tests. The DSP architecture spec lists FIR
   as the eventual target.
3. **24 ERB bands fixed.** Reasonable for V1; the band count is hard-coded
   in `AnalysisBank::setupForRate` and would only be revisited if
   validation suggests granularity is wrong.
4. **Single default preset.** Per `01-scope-freeze.md`. The few tunable
   numbers in `AnalysisBank` (sensitivity threshold mapping, attack/release
   time constants, harsh-zone threshold) are the levers used during M6.
5. **No automated REAPER track-1 swap.** The validation ReaScript renders
   bypass/wet for the slot currently on Track 1; you swap files manually
   between slots. Programmatic swap is a small future enhancement.

## Where to go next

- To rebuild and listen: `docs/build/01-windows-setup.md`.
- To run a validation pass: `docs/build/02-validation-runbook.md`.
- To understand DSP block intent: `docs/spec/03-dsp-architecture.md`.
- To understand acceptance gates: `docs/spec/02-measurements.md`,
  `docs/validation/04-validation-suite.md`.
