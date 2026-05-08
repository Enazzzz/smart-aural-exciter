# 02 - Measurements: CPU, Latency, Loudness, Safety

Status: **LOCKED for V1**.
Companion docs: `01-scope-freeze.md`, `03-dsp-architecture.md`, `04-validation-suite.md`.

This document fixes every numeric threshold V1 is graded against. If a number
needs to change, it must change here first; implementation, validation, and
release notes follow this file, not the other way around.

---

## 1. Reference test machine and host

To make CPU and latency numbers reproducible, every measurement in this doc
assumes the following baseline. Other machines / hosts can be used for
exploratory tests, but pass/fail decisions are made on this baseline.

| Item                | Reference value                                                       |
| ------------------- | --------------------------------------------------------------------- |
| OS                  | Windows 11 x64, current stable build, audio services optimized.       |
| CPU baseline class  | Modern desktop CPU, 6+ physical cores, base clock >= 3.0 GHz.         |
| RAM                 | >= 16 GB.                                                             |
| Audio interface     | ASIO-capable interface with stable round-trip < 10 ms at 256 buffer.  |
| Host                | REAPER current stable, 64-bit, VST3 scan enabled.                     |
| Project sample rate | 48 kHz unless a row explicitly specifies another rate.                |
| Project buffer size | 256 samples unless a row explicitly specifies another buffer.         |
| Power profile       | "High performance" / not battery-saver. CPU throttling disabled.     |

A short setup checklist mirroring this table lives in `04-validation-suite.md`.

---

## 2. CPU budget

CPU is measured with **one instance** of the plugin on the master bus of the
reference REAPER session (see `04-validation-suite.md`), reading REAPER's
"CPU use" meter as the primary indicator and Windows Task Manager as a
sanity check. Numbers are sustained averages over a 60-second loop, not peaks.

| Mode                       | Sample rate | Target CPU per instance | Hard ceiling |
| -------------------------- | ----------- | ----------------------- | ------------ |
| Standard quality           | 48 kHz      | <= 6 %                  | 8 %          |
| Standard quality           | 96 kHz      | <= 9 %                  | 12 %         |
| High quality               | 48 kHz      | <= 10 %                 | 12 %         |
| High quality               | 96 kHz      | <= 14 %                 | 18 %         |
| Bypassed (plugin enabled, internal bypass on) | any | <= 0.5 %  | 1 %          |

Pass criteria:

- All "Target" rows must hit their target on the reference machine.
- A row may exceed its target only if it is below its hard ceiling **and**
  there is a documented reason in the release notes.
- Going above any "Hard ceiling" is a V1 release blocker.

---

## 3. Latency budget

Latency is reported by the plugin to the host (PDC) and confirmed by a
loopback null test against the dry signal in REAPER.

| Mode             | Target latency        | Hard ceiling                    |
| ---------------- | --------------------- | ------------------------------- |
| Standard quality | 64–256 samples        | 384 samples                     |
| High quality     | 256–512 samples       | 1024 samples (with conditions)  |

Conditions for shipping a "High quality" mode above 512 samples:

- Blind-ish A/B in `04-validation-suite.md` shows a clear preference for the
  higher-latency variant on at least 60% of the reference set; **and**
- The added latency is fully reported via PDC; **and**
- The latency does not exceed 1024 samples at 48 kHz.

If those conditions are not met, V1 ships with `High quality` capped at
512 samples and the higher-latency variant is deferred.

Additional latency rules:

- Latency must be deterministic for a given (sample rate, quality mode).
- Latency must not change while audio is running. Mode switches that change
  latency must request a host-side PDC reset rather than mutating buffers
  mid-stream.

---

## 4. Loudness, gain, and headroom

Loudness is measured with an integrated LUFS meter (e.g. ReaLoudness Meter
or any ITU-R BS.1770-4-compliant meter) over a 60-second loop on each
reference track from `04-validation-suite.md`.

| Property                                         | Target / rule                            |
| ------------------------------------------------ | ---------------------------------------- |
| Default-preset integrated loudness delta vs bypass | -0.5…+1.0 LUFS, mean across reference set |
| Per-track loudness delta vs bypass                 | <= +1.5 LUFS on any single reference track |
| True-peak delta vs bypass at default preset        | <= +1.0 dBTP on any single reference track |
| Internal headroom before output stage              | -3 dB by default (advanced trim configurable) |

Rationale: a transparent exciter should not act as a loudness maximizer.
Anything > +1.5 LUFS on a single track means the perceptual model is
over-driving and must be retuned.

---

## 5. Output safety stage

The output safety stage is the very last block before the plugin's audio
leaves to the host. Its rules are not user-facing trade-offs; they are
guardrails.

| Property                            | Value                                                       |
| ----------------------------------- | ----------------------------------------------------------- |
| Internal hard ceiling               | -1.0 dBFS sample peak                                       |
| Internal hard true-peak ceiling     | -1.0 dBTP                                                   |
| Behavior at ceiling                 | Soft-knee limit; no hard digital clipping.                  |
| Maximum allowed sample-peak overshoot | 0.0 dBFS for <= 1 sample (transient slip), never more.    |
| Maximum allowed true-peak overshoot | -0.3 dBTP under sustained worst-case input.                 |
| Behavior at extreme overload        | Output gracefully limits; no NaN, no Inf, no DC offset.    |

DC blocking and denormal protection are required and must be active in all
modes.

Pass criteria:

- Internal stress test (see `04-validation-suite.md` section "Safety stress")
  must show **zero** samples above -1.0 dBFS at the plugin output across all
  reference tracks at default preset.
- Stress with a +12 dB input pre-gain to the plugin must still respect the
  -1.0 dBTP ceiling within the documented overshoot allowance.

---

## 6. Stability and reliability

| Property                                 | Requirement                                       |
| ---------------------------------------- | ------------------------------------------------- |
| Bypass click/pop                         | Inaudible at default preset on reference set.     |
| Parameter automation click/pop           | Inaudible for ramp times >= 5 ms.                 |
| Sample-rate change handling              | No crash, no audio glitch beyond 1 buffer.        |
| Buffer-size change handling              | No crash, no glitch beyond the new buffer length. |
| Soak test                                | 60 minutes default preset, no crash, no leak > 5 MB resident growth. |
| Plugin scan time                         | <= 1.5 seconds in REAPER cold scan.               |
| GUI open/close cycles                    | 1000 cycles without leak, crash, or visible lag.  |

---

## 7. Numerical determinism

For any fixed (input file, sample rate, plugin state, quality mode) the
offline rendered output must be **bit-identical** between runs on the same
binary and reference machine. This makes regression testing meaningful.

If multithreading inside the DSP path would break determinism, V1 must run
DSP single-threaded per instance and accept the CPU cost. The CPU budget
above already assumes this.

---

## 8. Profiling and measurement procedure (summary)

The full step-by-step protocol lives in `04-validation-suite.md`. Quick
reminder of the order operations must follow so numbers are comparable:

1. Set REAPER project to `48 kHz, 256 samples, render path = master bus`.
2. Insert plugin on master bus, advanced panel hidden, default preset.
3. Verify quality mode and any non-default advanced settings.
4. Loop the reference test track for >= 60 seconds.
5. Read CPU from REAPER's "CPU use" meter (sustained average, not spike).
6. Capture latency from REAPER's plugin info pane and confirm via null test.
7. Render the loop offline and run loudness + true-peak analysis.
8. Record numbers in `docs/validation/` per-track sheets.

---

Frozen on: 2026-05-07.
Last reviewed: 2026-05-07.
