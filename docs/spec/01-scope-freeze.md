# 01 - V1 Scope Freeze

Status: **LOCKED for V1**.
Owner: project lead (you).
Companion docs: `02-measurements.md`, `03-dsp-architecture.md`, `04-validation-suite.md`.

This document freezes the V1 scope so that implementation choices cannot quietly
expand. Anything not explicitly listed under "In scope" is **out of scope** for V1
and must wait for a post-V1 release. To change scope, edit this document
explicitly and bump the date in the footer.

---

## 1. Product identity

- **Name (working):** Smart Exciter (final name TBD post-V1).
- **Category:** Mastering-grade harmonic enhancer / aural exciter.
- **Differentiator:** Perceptual model decides *where* harmonics are added,
  rather than blanket high-shelf saturation.
- **Primary tagline:** "Adds detail your ear is already expecting."

## 2. Target user and primary workflow

- **Primary user:** experienced producer / critical listener (you), comfortable
  with mastering-bus tools.
- **Secondary user:** intermediate producers who want a "set and forget"
  master-bus enhancer.
- **Primary workflow:** insert as the *last or near-last* plugin on the
  stereo master bus in REAPER, after EQ/compression but before any final
  brickwall limiter.

## 3. Locked release constraints

| Decision               | V1 value                          | Notes                                                      |
| ---------------------- | --------------------------------- | ---------------------------------------------------------- |
| Format                 | VST3 only                         | No AU, no AAX, no CLAP, no LV2 in V1.                      |
| Platform               | Windows x64 only                  | No macOS, no Linux in V1.                                  |
| Channel layout         | Stereo only                       | No mono, no surround, no Atmos in V1.                      |
| Sample-rate support    | 44.1, 48, 88.2, 96, 176.4, 192 kHz| Higher rates may be allowed but are not validated.         |
| License model          | Free download                     | Paid/pro tier deferred to post-V1.                         |
| Copy protection        | None                              | No license key, no activation server, no online check.     |
| Telemetry              | None                              | No analytics, no phone-home, no crash reporter in V1.      |
| Update channel         | Manual download                   | No in-plugin update check in V1.                           |

## 4. Locked sonic identity

- **Default character:** transparent / subtle. The bypass A/B should reveal
  enhancement, not a tonal makeover.
- **Calibration target:** electronic-first (Aphex Twin–style detail and
  transient air), with rock as the secondary check genre.
- **Acceptable side effects:** very mild perceived loudness lift, minor
  apparent stereo width opening from added air content.
- **Forbidden side effects:** harsh sibilance, ice-pick top end,
  cymbal/hi-hat exaggeration, audible aliasing, audible pumping.

## 5. Locked feature set (in scope for V1)

### 5.1 DSP features

- Selective harmonic generation driven by per-band perceptual analysis.
- Perceptual gating that scales excitation by local masking and existing
  spectral energy.
- Anti-harshness guard that limits excitation in the 2–6 kHz "ear-pain"
  region when input energy there is already high.
- Output safety stage with a hard internal ceiling (see `02-measurements.md`).
- Stable bypass and parameter automation without clicks or pops.

### 5.2 User interface — simple panel

Exactly five top-level controls. No more, no fewer in V1.

| Control       | Range / type      | Purpose                                                  |
| ------------- | ----------------- | -------------------------------------------------------- |
| `Amount`      | 0–100, default 35 | Global excitation intensity.                             |
| `Tone`        | -50…+50, default 0| Tilts excited-region center darker / brighter.           |
| `Sensitivity` | 0–100, default 50 | Analysis threshold; how aggressively the gate opens.     |
| `Mix`         | 0–100, default 100| Dry/wet blend (V1 default is fully wet).                 |
| `Output`      | -12…+6 dB, def 0  | Post-process trim before the safety stage.               |

### 5.3 User interface — advanced panel

Hidden behind a disclosure ("Advanced" toggle). Defaults are tuned to be safe
so users can ignore it forever.

- Quality mode selector: `Standard` (lower latency) | `High` (more samples
  of latency, only enabled when audibly better — see `02-measurements.md`).
- Anti-harshness guard amount: 0–100, default 60.
- Activity-view scaling: linear / log / auto, default auto.
- Internal headroom trim: -6…0 dB, default -3 dB.

### 5.4 Visual feedback

Both required in V1:

- **Basic metering:** input level, output level, and gain delta, peak + RMS.
- **Enhancement activity view:** band-resolved indicator showing where
  harmonic emphasis is currently being applied. Refresh rate ~30 Hz.

### 5.5 Preset management

- One preset only in V1: the carefully tuned default that loads on insert.
- Standard VST3 host preset save/load is supported.
- No factory preset browser, no genre packs, no preset marketplace in V1.

## 6. Out of scope for V1 (deferred)

These items are explicitly **deferred** so V1 can ship:

- macOS, Linux builds.
- AU, AAX, CLAP, LV2 formats.
- Mono / surround / Atmos channel layouts.
- Track-insert specialized profiles (vocals / drums / synth).
- Genre-specific preset packs.
- AI auto-mastering or auto-amount features.
- Sidechain input / external key.
- Mid/Side processing controls.
- M/S width controls beyond what natural excitation produces.
- Loudness-normalized A/B (LUFS-matched bypass).
- Spectrum analyzer overlay.
- Tooltips with deep-link documentation.
- In-plugin manual or help panel.
- License key / online activation.
- Telemetry / crash reporting.
- Update notifier.
- Localization (V1 ships English only).
- Accessibility certification (basic keyboard/screen-reader hooks only).

## 7. Definition of done for V1

V1 is "done" when **all** of the following are true:

1. Installer / drop-in VST3 loads in REAPER on Windows x64 without errors.
2. All five simple controls plus the four advanced controls behave per spec.
3. All measurable acceptance criteria in `02-measurements.md` are met.
4. The validation listening protocol in `04-validation-suite.md` has been
   run end-to-end and passes its gates.
5. No DAW crash in a 1-hour soak test with default preset on the master bus.
6. The default preset is preferred or tied vs bypass on a majority of the
   reference audio set in blind-ish A/B (see `04-validation-suite.md`).

## 8. Naming conventions used across spec docs

- "V1" = the first public release of the plugin.
- "Default preset" = the single preset shipped in V1, loaded on instantiation.
- "Master bus" = the final stereo bus before the host's main output, after
  any user-applied EQ / compression and before any final brickwall limiter.
- "Activity view" = the band-resolved harmonic-emphasis visualization.

---

Frozen on: 2026-05-07.
Last reviewed: 2026-05-07.
