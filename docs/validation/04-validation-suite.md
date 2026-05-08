# 04 - Validation Suite (REAPER)

Status: **LOCKED for V1.**
Companion docs: `../spec/01-scope-freeze.md`, `../spec/02-measurements.md`,
`../spec/03-dsp-architecture.md`.

This document defines the reference audio corpus, the repeatable REAPER
session layout, and the A/B + measurement protocol used to validate the
plugin against the V1 acceptance criteria.

---

## 1. Reference audio corpus

The corpus is the fixed set of stereo full-mix files the plugin is judged
against. Tracks live in:

```
docs/validation/refs/
  electronic/
  rock/
  vocal/
  stress/
```

### 1.1 Required minimum set (per `01-scope-freeze.md`)

| Slot | Category   | Description                                                | Why it's in the set                            |
| ---- | ---------- | ---------------------------------------------------------- | ---------------------------------------------- |
| E1   | electronic | Aphex-style detail-rich production, dense transients.      | Primary calibration target.                    |
| E2   | electronic | Modern tightly-mastered electronic, broad spectrum.        | Stresses the perceptual gate vs hyped masters. |
| E3   | electronic | Sparse / ambient electronic with long tails.               | Reveals pumping, denormals, low-level artifacts.|
| R1   | rock       | Drum-forward modern rock, loud guitars.                    | Tests harshness guard on cymbal-heavy content. |
| R2   | rock       | Vintage / dynamic rock with natural drums.                 | Tests transparency on already-natural mixes.   |
| V1   | vocal      | Vocal-forward pop or singer-songwriter, lead vocal central.| Tests sibilance behavior in 2–6 kHz region.    |
| S1   | stress     | Already-bright / harsh master.                             | Worst case for ear-pain risk.                  |

### 1.2 Sourcing rules

- Each track is a stereo WAV or FLAC, sample rate 48 kHz preferred (44.1 kHz
  acceptable), 24-bit float or integer.
- Track length 60–180 seconds. If a song is longer, an excerpt is fine —
  capture the most representative chorus or section.
- Loudness is **not** normalized in the corpus itself. The protocol below
  level-matches at evaluation time, not at storage time.
- File naming:
  `EnnnVnn_<short_slug>.wav`, e.g. `E101_aphex_excerpt.wav`,
  `R201_dyn_rock.wav`. The numeric pair lets the corpus grow without
  renaming existing files.
- Tracks themselves are **not** committed to this repository (licensing).
  The folder structure is committed; a `corpus.txt` index file lists the
  expected file names and short descriptions per slot.

### 1.3 Corpus index template

Create `docs/validation/refs/corpus.txt` with rows like:

```
E101  electronic  aphex_excerpt.wav         48000  Aphex-style dense detail
E201  electronic  modern_tight_master.wav   48000  Loud broad-spectrum
E301  electronic  sparse_ambient.wav        48000  Long tails / low level
R101  rock        modern_drum_forward.wav   48000  Cymbal-rich
R201  rock        vintage_dynamic.wav       44100  Natural drums
V101  vocal       vocal_forward.wav         48000  Sibilant-prone
S101  stress      bright_harsh_master.wav   48000  Worst-case bright
```

If a slot is missing on disk, the validation run for that slot is recorded
as `SKIP`, not `PASS`. V1 cannot be released with > 1 slot in `SKIP`.

## 2. Reference REAPER session

### 2.1 Project settings (must match `02-measurements.md`)

- Sample rate: **48 kHz**.
- Buffer size: **256 samples** via the configured ASIO interface.
- Project bit depth (render): 32-bit float WAV.
- Master channel layout: stereo.
- Pre-fader metering, peak + RMS visible.
- Disable any host-level dither for offline renders.

### 2.2 Track layout

```
Track 1: REF (the reference audio file, one slot at a time)
Track 2: BYPASS bounce (rendered, used for level-matched A/B)
Track 3: WET    bounce (rendered, used for level-matched A/B)
Master:  insert -> Smart Exciter (default preset, advanced panel hidden)
         insert -> ITU-R BS.1770-4-compliant LUFS / true-peak meter (last)
```

Save this session as `docs/validation/reaper/v1-validation.RPP`. Do not
commit any audio bounces; only commit the project file and notes.

### 2.3 Per-slot rendering procedure

1. Replace the file on Track 1 with the slot under test.
2. Bypass the plugin internally (use the plugin's bypass, not the host's,
   so PDC is preserved). Render to `Track 2` -> file `<slot>_bypass.wav`.
3. Enable the plugin (default preset). Render to `Track 3` -> file
   `<slot>_wet.wav`.
4. Both renders should be 60–180 seconds matching the reference clip.

## 3. Measurement protocol

For every slot in the corpus, run the following in order. Use one row per
slot in the per-slot scorecard (template in section 6).

### 3.1 CPU and latency

1. Confirm `02-measurements.md §1` reference machine setup.
2. With Standard quality + default preset, loop the slot for 60 seconds.
3. Record sustained REAPER CPU % for: bypassed, Standard, High quality.
4. Record reported plugin latency (samples) for Standard and High quality.
5. Confirm latency via null test:
   - Sum (`bypass`) and (`wet` with `Mix=0`) inverted.
   - Result must be -inf or near digital silence aside from the safety
     stage's de minimis residual.

Pass = numbers within targets/ceilings in `02-measurements.md`.

### 3.2 Loudness and true-peak

1. Use the inserted ITU-R BS.1770-4-compliant meter on the master.
2. Measure integrated LUFS over the full slot for `<slot>_bypass.wav` and
   `<slot>_wet.wav`.
3. Measure max true-peak (dBTP) for each.
4. Record deltas (`wet - bypass`) for both LUFS and dBTP.

Pass = within budgets in `02-measurements.md §4` and `§5`.

### 3.3 Safety stress

1. Add a +12 dB gain plugin **before** the Smart Exciter on the master.
2. Render `<slot>_stress.wav` with the plugin enabled, default preset.
3. Inspect the rendered file's sample peak and true-peak.

Pass = no sample > -1.0 dBFS, true-peak overshoot within `02-measurements.md §5`.

### 3.4 Transparency / quality A/B (blind-ish)

1. In a separate REAPER project, place `<slot>_bypass.wav` and `<slot>_wet.wav`
   on two muted tracks behind a third "router" track.
2. Use REAPER's track grouping or a simple JSFX A/B switcher to swap them
   without revealing which is which.
3. Level-match by ear within ~0.3 dB before doing the A/B.
4. Listen on at least:
   - Studio monitors (if available),
   - One pair of mid-range headphones,
   - One consumer device (laptop speaker or earbuds).
5. For each listening pass, record one of:
   `PREFER_WET`, `TIE`, `PREFER_BYPASS`, plus a one-line note.

Pass thresholds for the corpus as a whole:

- Default-preset wet is `PREFER_WET` or `TIE` on **>= 60%** of slot/listening
  combinations across the required minimum set.
- No slot may produce `PREFER_BYPASS` on **all** listening environments —
  if a slot is bypass-preferred everywhere, that's a tuning regression.

### 3.5 Long-pass ear fatigue check

Once the per-slot tests pass, do a continuous 20-minute mixed-program
listening pass with the plugin engaged at default preset on a varied
playlist drawn from the corpus. Record one line:

- `OK`: no fatigue, would leave it on for a full session.
- `MILD`: tolerable, but tone drift / harshness noticeable over time.
- `BAD`: had to bypass before 20 minutes were up.

Pass = `OK`. `MILD` is a soft fail, `BAD` is a hard fail.

### 3.6 Non-audiophile spot check

At least once per validation cycle (one round per V1 release candidate),
play `<slot>_bypass.wav` and `<slot>_wet.wav` for someone who isn't a
critical listener, blinded as A/B. Record their preference for >= 2 slots
(electronic + rock or vocal). This is informational, not a pass/fail gate.

## 4. Regression suite (offline)

To make iteration cheap, store golden offline renders of every slot at
default preset, both Standard and High quality, after each accepted DSP
change. The regression check on a new build is:

1. Render every slot through both quality modes.
2. Compare sample-bit-identical to the golden render.
3. Any differing sample is a regression unless the change is intentional,
   in which case the goldens are refreshed and the diff is recorded in
   release notes.

Bit-determinism is required by `02-measurements.md §7`, which makes this
test meaningful. If the implementation later needs nondeterminism (e.g.
threaded analysis), this test changes from "bit-identical" to "energy and
LUFS within an explicit tolerance" and that change is recorded in
`02-measurements.md`.

## 5. Acceptance gate (V1 release)

V1 ships when, on the reference machine and the REAPER project above,
**all** of the following pass:

- [ ] All required slots are present (no `SKIP` rows).
- [ ] CPU within targets / ceilings on every slot.
- [ ] Latency within targets / ceilings on every slot.
- [ ] Loudness and true-peak deltas within budgets on every slot.
- [ ] Safety stress test shows zero ceiling violations on every slot.
- [ ] A/B preference >= 60% `PREFER_WET` or `TIE` across the corpus.
- [ ] No slot is `PREFER_BYPASS` on all three listening environments.
- [ ] Long-pass fatigue test = `OK`.
- [ ] Bit-identical regression vs current golden renders (or documented
      delta).
- [ ] 60-minute soak test in REAPER on the master bus, no crash, no
      memory growth > 5 MB.

## 6. Per-slot scorecard template

Copy this block per slot under test, e.g. into
`docs/validation/scorecards/E101.md`:

```markdown
# Validation scorecard - <SLOT>

- Slot: E101
- File: aphex_excerpt.wav
- Date: YYYY-MM-DD
- Build: <git sha or build label>

## CPU / latency

|                | Standard | High |
| -------------- | -------- | ---- |
| CPU % (60s avg)|          |      |
| Latency (samp) |          |      |
| Null test      | PASS / FAIL |   |

## Loudness / true-peak

|                          | bypass | wet | delta |
| ------------------------ | ------ | --- | ----- |
| Integrated LUFS          |        |     |       |
| Max true-peak (dBTP)     |        |     |       |

## Safety stress (+12 dB pre-gain)

- Sample peak: ____ dBFS  (must be <= -1.0)
- True-peak:   ____ dBTP  (must be within budget)

## A/B (blind-ish, level matched)

|                | Monitors | Headphones | Consumer |
| -------------- | -------- | ---------- | -------- |
| Preference     |          |            |          |
| One-line note  |          |            |          |

## Notes

(observations, retune candidates, regressions vs prior build)
```

## 7. Cadence

- Run the **full** suite (all slots, all sections) before any release
  candidate.
- Run the **regression-only** suite (section 4) on every DSP change.
- Run the **A/B + fatigue** suite (sections 3.4, 3.5) at least once per
  week during active tuning.

---

Last reviewed: 2026-05-07.
