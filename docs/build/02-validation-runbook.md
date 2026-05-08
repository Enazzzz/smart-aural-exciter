# 02 - Validation Runbook (M6)

This is the operational runbook for the V1 validation gate. It folds together:

- the spec gates in [docs/spec/02-measurements.md](../spec/02-measurements.md),
- the protocol in [docs/validation/04-validation-suite.md](../validation/04-validation-suite.md),
- the build/install path in [01-windows-setup.md](01-windows-setup.md),
- and the helper scripts under `tools/`.

You execute it; the tooling makes each loop cheap.

## TL;DR loop

For each tuning iteration:

1. Build + install the latest plugin.
2. Open the validation REAPER project, replace Track 1 with the slot under
   test, run the batch render script.
3. Run the loudness measurement script and inspect deltas + ceiling status.
4. Do blind-ish A/B + fatigue listening per the validation suite, fill in
   the per-slot scorecards.
5. Decide: tune DSP and loop, or declare V1 ready when the gate passes.

## One-time setup

1. Install build prereqs (see [01-windows-setup.md](01-windows-setup.md)).
2. Install the validation toolchain:
	```powershell
	pip install numpy pyloudnorm soundfile
	```
3. Drop the seven required reference audio files into the right
   sub-folders under `docs/validation/refs/<category>/`. Filenames must
   match `docs/validation/refs/corpus.txt`.
4. Open REAPER and create the validation session at
   `docs/validation/reaper/v1-validation.RPP` per
   [04-validation-suite.md section 2](../validation/04-validation-suite.md#2-reference-reaper-session).
5. Insert ReaLoudness Meter (or any BS.1770-4-compliant LUFS meter) at
   the very end of the master FX chain.

## Per-iteration loop

### Step 1 - Build and install

```powershell
powershell -ExecutionPolicy Bypass -File tools\build\build_and_install.ps1
```

REAPER picks up the new VST3 on next plugin scan.

### Step 2 - Batch render the corpus

In REAPER:

1. Open `docs/validation/reaper/v1-validation.RPP`.
2. Action list -> load `tools/reaper/render_corpus.lua` once.
3. For each slot:
	- Drag the slot's audio file onto Track 1 (REF).
	- Run "Script: render_corpus.lua".
	- The script renders `<slot>_bypass.wav`, `<slot>_wet.wav`, and
	  prompts you to enable a +12 dB pre-gain plugin for the
	  `<slot>_stress.wav` render.

(Future enhancement: extend the ReaScript to swap Track 1's media item
programmatically and toggle the +12 dB plugin by name. Out of scope for V1.)

Renders land at `docs/validation/reaper/renders/<slot>/`. That folder is
gitignored.

### Step 3 - Measure loudness, peak, and true-peak

```powershell
python tools\measure\measure_loudness.py
```

The script prints a table per slot with deltas vs bypass and flags any:

- `[LUFS-DELTA-EXCEEDED]`  - wet vs bypass > +1.5 LUFS on a single track
- `[TP-DELTA-EXCEEDED]`    - wet vs bypass > +1.0 dBTP on a single track
- `[CEILING-VIOLATION]`    - any sample > -1.0 dBFS at plugin output

Any flag means the DSP needs tuning - go back to step 1 after editing
coefficients (typically lower default `Amount`, raise default `guardAmount`,
or tighten the harshness threshold in `AnalysisBank::runOneHop`).

Optionally write a CSV for record keeping:

```powershell
python tools\measure\measure_loudness.py --csv docs\validation\scorecards\latest_metrics.csv
```

### Step 4 - Listen and score

Per [04-validation-suite.md sections 3.4 / 3.5](../validation/04-validation-suite.md#34-transparency--quality-ab-blind-ish):

1. In a separate REAPER project, A/B `<slot>_bypass.wav` vs `<slot>_wet.wav`
   level-matched within ~0.3 dB.
2. Listen on monitors, headphones, and a consumer device.
3. Score `PREFER_WET` / `TIE` / `PREFER_BYPASS` per environment per slot,
   plus a one-line note. Save into
   `docs/validation/scorecards/<slot>.md` (start by copying
   `docs/validation/scorecards/TEMPLATE.md`).
4. Once per release candidate, do the 20-minute mixed-program fatigue
   pass and the non-audiophile spot check.

### Step 5 - Pass/fail decision

Use [04-validation-suite.md section 5](../validation/04-validation-suite.md#5-acceptance-gate-v1-release)
as the actual checklist. The build is V1-ready when ALL boxes are ticked.
Any failure -> tune and loop.

## Where each measurement target lives in code

When tuning, these are the most likely files to edit:

| Symptom                                  | First place to look                                                  |
| ---------------------------------------- | -------------------------------------------------------------------- |
| Default too subtle                        | `AnalysisBank::runOneHop` (sigmoid gate, sensThresholdDb mapping)    |
| Default too aggressive on bright masters  | `HarshnessGuard::process` (kMinGuardFloor), AnalysisBank harsh threshold |
| Ceiling violation under stress            | `SafetyLimiter::process` (knee width, attack)                        |
| LUFS jump too high                        | `HarmonicGenerator::process` (saturator gain), default Amount        |
| Tone parameter doesn't feel useful        | `AnalysisBank::runOneHop` (toneWeight ramp)                          |
| Pumping / audible smoothing                | `AnalysisBank::setupForRate` (attackCoef_ / releaseCoef_)            |
| Block-edge clicks on transient material   | `HarmonicGenerator::process` (per-block FFT limitation, see header)   |

## Determinism check (cheap regression)

Run between any DSP edit:

```powershell
ctest --test-dir build -C Release --output-on-failure
```

`test_determinism` will catch any accidental nondeterminism added to the
audio path. `test_safety_limiter` will catch ceiling regressions.
`test_erb_layout` is a sanity check that survives most refactors.

## Done criteria reminder

This entire runbook exists to satisfy
[04-validation-suite.md section 5](../validation/04-validation-suite.md#5-acceptance-gate-v1-release).
That checklist is canonical; this runbook is just the execution path.
