# 03 - DSP Architecture

Status: **DRAFT for V1 implementation planning.** Block-level decisions are
locked; coefficient/window choices are tunable during implementation.
Companion docs: `01-scope-freeze.md`, `02-measurements.md`, `04-validation-suite.md`.

This document specifies the V1 DSP architecture: every block in the signal
chain, what each block does, what it consumes, what it produces, and the
constraints from `02-measurements.md` it must respect.

---

## 1. Architectural goals (recap)

The DSP must:

1. Add harmonics **selectively**, only where the listener's ear actually
   benefits, not as a global high-shelf saturation.
2. Stay transparent at default settings (no obvious tonal makeover).
3. Respect the CPU, latency, loudness, and safety budgets in
   `02-measurements.md`.
4. Be deterministic and bit-reproducible offline.
5. Be safe by default: the worst possible knob combination at the worst
   possible input must still not blow past the output safety stage.

## 2. High-level signal flow

```mermaid
flowchart LR
    in[Stereo input] --> dcblock[DC block + denormal guard]
    dcblock --> oversample[Oversampler"<br/>Standard: 2x<br/>HighQuality: 4x"]
    oversample --> split[Tap dry path]
    split --> analysis[Perceptual Analysis Bank]
    split --> harmonic[Selective Harmonic Generator]
    analysis -->|per-band gain envelopes| harmonic
    analysis -->|harshness mask| guard[Anti-Harshness Guard]
    harmonic --> guard
    guard --> wetSum[Wet sum"<br/>per-band recombine"]
    wetSum --> drywet[Dry/Wet mix"<br/>aligned to dry tap"]
    split -->|aligned dry| drywet
    drywet --> trim[Output trim"<br/>+ headroom trim"]
    trim --> downsample[Downsampler]
    downsample --> safety[Safety stage"<br/>soft-knee true-peak limit"]
    safety --> out[Stereo output]
```

Notes on the diagram:

- The `Tap dry path` makes a sample-accurate copy of the post-oversample
  signal so the dry/wet mix doesn't move in time relative to the wet signal.
- The dry path that returns into `Dry/Wet mix` is delay-aligned to whatever
  group delay the analysis + harmonic + guard chain introduces.
- `Oversampler` and `Downsampler` are the same FIR design (linear-phase)
  used as a matched pair so latency is symmetric and predictable.

## 3. Block-by-block specification

### 3.1 DC block + denormal guard

- Purpose: remove sub-audio DC; keep CPU stable on near-zero buffers.
- Implementation: 1st-order high-pass at ~5 Hz, plus add-and-subtract
  tiny-noise denormal protection (or `flush-to-zero` MXCSR equivalent).
- Latency: 0 samples (IIR).
- Inputs: raw host buffer.
- Outputs: cleaned stereo signal.

### 3.2 Oversampler

- Purpose: give the analysis and nonlinear stages headroom above 20 kHz so
  newly generated harmonics don't alias back into the audible band.
- Modes:
  - `Standard quality`: 2x oversampling.
  - `High quality`: 4x oversampling.
- Implementation: linear-phase polyphase FIR, designed for >= 100 dB stop-band
  attenuation above the source Nyquist.
- Latency: see `02-measurements.md`. Reported via PDC.

### 3.3 Perceptual Analysis Bank

This is the heart of the "smart" behavior. It does not produce audio; it
produces **control signals** for the harmonic generator and the harshness
guard.

- Purpose: estimate, per critical band, how much harmonic enhancement the
  listener's ear would actually benefit from given the current spectral
  content.
- Inputs: oversampled stereo signal.
- Outputs:
  - `bandGain[N]`: per-band excitation gain in linear units, range 0..1.
  - `harshnessMask[N]`: per-band attenuation factor for the anti-harshness
    guard, range 0..1.
  - `analysisOk`: boolean indicating analysis is producing valid output
    (false during warmup, after sample-rate change, etc).

- Implementation outline:
  - Short-time analysis on overlapping windows (size and hop chosen during
    implementation to fit the latency budget; default starting point: 512
    samples at 48 kHz with 50% overlap, scaled with sample rate).
  - Map bin energy onto an ERB-scaled (perceptual) band layout, ~24 bands.
  - Estimate per-band tonal vs noisy character; tonal bands get more
    excitation, noisy bands get less.
  - Estimate per-band masking: if a band's neighbors are already loud, the
    ear masks added harmonics there, so excitation is reduced rather than
    increased.
  - Estimate per-band "ear-pain risk" in the 2–6 kHz region: when in-band
    energy is already above an internal threshold, contribute to
    `harshnessMask` to throttle the guard later in the chain.
  - Smooth all outputs in time with attack/release envelopes tuned for
    "no audible pumping" rather than "fast reaction." Defaults are tunable
    during implementation; starting point: 5 ms attack, 80 ms release.

- Constraints:
  - Must run in fixed-cost time per block (no allocations on the audio
    thread).
  - Must be bit-deterministic given the same input and parameters.
  - Must respect `Sensitivity` and `Tone` parameters (they bias thresholds
    and band weighting respectively).

### 3.4 Selective Harmonic Generator

- Purpose: produce the wet harmonic content, modulated per-band by
  `bandGain[N]`.
- Inputs: oversampled stereo signal, `bandGain[N]`, `Amount` parameter.
- Outputs: per-band wet signal ready for the harshness guard.

- Implementation outline:
  - Filter bank that splits the input into the same N perceptual bands the
    analysis used.
  - Each band passes through a mild nonlinearity (e.g. soft-asymmetric
    waveshaper) selected and tuned per-band so that dark bands get warmer
    even-order content and bright bands get airier odd-order content.
  - Each band's wet output is multiplied by `bandGain[band] * Amount`.
  - Bands recombine in `Wet sum` later — they are kept separate up through
    the harshness guard so the guard can attenuate per-band.

- Constraints:
  - All nonlinearities operate on the oversampled signal so that any new
    harmonic that exceeds the source Nyquist is filtered out by the
    downsampler.
  - No band may, on its own at default settings, contribute more than the
    per-band loudness delta budget implied by `02-measurements.md`.

### 3.5 Anti-Harshness Guard

- Purpose: enforce the "no ice-pick" requirement from `01-scope-freeze.md`,
  even if the user pushes `Amount` and `Sensitivity` aggressively.
- Inputs: per-band wet signal from the harmonic generator,
  `harshnessMask[N]`, advanced `anti-harshness guard amount` parameter.
- Outputs: per-band wet signal with the harshness-prone bands attenuated.

- Implementation outline:
  - Apply `attenuationBand = 1 - guardAmount * harshnessMask[band]` to each
    band before the wet sum.
  - The guard is *always on* even when the advanced parameter is 0, but
    with a small minimum floor so a never-touched advanced panel still
    protects the listener.

### 3.6 Wet sum and dry alignment

- `Wet sum` recombines per-band wet signals into a single stereo wet bus
  using the same band-edge filters used in the split (matched
  reconstruction so the band split itself does not color the dry-when-bypassed
  case).
- The dry tap from `Tap dry path` is delayed by the exact group delay of
  `Perceptual Analysis Bank + Selective Harmonic Generator + Anti-Harshness
  Guard + Wet sum`.
- This delay is fixed for a given (sample rate, quality mode) so PDC can
  report it without lying.

### 3.7 Dry/Wet mix

- Equal-gain blend, controlled by `Mix`.
- Default `Mix` = 100 (fully wet), per `01-scope-freeze.md`.
- The wet path always passes through the dry path's exact delay; the wet
  itself is **already** delay-aligned upstream so this block is a simple
  per-sample crossfade.

### 3.8 Output trim and headroom trim

- `Output` parameter applies a -12…+6 dB gain.
- Advanced `internal headroom trim` applies a -6…0 dB gain, default -3 dB,
  *before* the output gain. This gives the safety stage room to work
  without forcing the user to set output negative.

### 3.9 Downsampler

- Purpose: return the signal to host sample rate.
- Implementation: matched pair of the oversampler FIR.
- Latency: matched to oversampler; full chain latency reported via PDC.

### 3.10 Safety stage (true-peak soft-knee limiter)

- Purpose: enforce the -1.0 dBFS / -1.0 dBTP ceiling from `02-measurements.md`.
- Implementation:
  - True-peak detection via short-window upsampled peak estimation
    (oversampling factor matched to whatever the chain already runs at, at
    minimum 4x).
  - Soft-knee gain reduction starting ~1 dB below ceiling.
  - Auto-release tuned to be inaudible on transient material.
- Behavior on extreme overload: graceful gain reduction, no NaN/Inf, no DC
  offset, no DSP path "frozen" state (must continue to recover within tens
  of milliseconds when input drops).

## 4. Parameter wiring

| User-facing control       | Internal effect                                                      |
| ------------------------- | -------------------------------------------------------------------- |
| `Amount`                  | Multiplies post-`bandGain` excitation gain.                          |
| `Tone`                    | Tilts band weighting in the analysis bank (negative = darker).       |
| `Sensitivity`             | Lowers the analysis threshold (more bands open up faster).           |
| `Mix`                     | Crossfades final stereo wet vs delay-aligned dry.                    |
| `Output`                  | Pre-safety output gain.                                              |
| Quality mode              | Switches oversampler / downsampler factor and analysis window length.|
| Anti-harshness guard amt  | Scales the per-band `harshnessMask` strength.                        |
| Activity-view scaling     | UI-only; does not affect audio.                                      |
| Internal headroom trim    | Pre-output static gain trim.                                         |

Parameter ramp behavior:

- Every audio-affecting parameter must ramp over >= 5 ms when changed,
  per `02-measurements.md`.
- Mode switches that change latency request a host PDC update and ramp
  audibly-affecting state across a buffer boundary.

## 5. Threading and allocation rules

- The audio thread allocates **nothing**. All buffers are sized in
  `prepareToPlay` / equivalent and reused.
- The analysis bank, harmonic generator, and harshness guard run on the
  audio thread. No worker threads in V1.
- The activity-view UI reads the latest published `bandGain[N]` snapshot
  via a lock-free atomic swap or single-writer / single-reader queue. The
  audio thread never blocks on the UI.
- All math runs in 32-bit float on the audio thread; analysis may use
  64-bit float internally where it materially helps stability.

## 6. Failure modes and fallbacks

| Condition                                            | Fallback behavior                                       |
| ---------------------------------------------------- | ------------------------------------------------------- |
| Analysis bank not yet warmed up                      | Pass-through (mix is forced to dry until `analysisOk`). |
| Sample rate changes mid-session                      | Re-init chain, request PDC update, brief mute (<= 1 buffer). |
| Buffer size changes mid-session                      | Re-init buffers, no glitch beyond new buffer length.    |
| Downsampler detects NaN/Inf                          | Replace with silence for affected samples, log internally. |
| Safety stage detects sustained > -1.0 dBTP overshoot | Increase gain reduction aggression, never let through.  |

## 7. Open implementation-time decisions

These are intentionally left open to be decided during implementation, with
guardrails:

- Exact analysis window size and hop. **Guardrail:** must fit inside the
  latency budget in `02-measurements.md`.
- Exact filter-bank topology (e.g. complementary IIR vs polyphase FFT).
  **Guardrail:** band split + recombine must be near-perfect-reconstruction
  in dry-signal terms.
- Exact per-band waveshaper curve set. **Guardrail:** cannot make any
  reference track exceed the loudness or true-peak deltas in
  `02-measurements.md`.
- Whether to ship `High quality` at 4x or to test 8x in addition.
  **Guardrail:** decision criteria are spelled out in `02-measurements.md §3`.
- Whether the safety stage uses an internal lookahead or a release-only
  design. **Guardrail:** total reported PDC must stay within the latency
  ceiling for the chosen mode.

These decisions go in the implementation plan that follows this spec, not
in this document.

---

Last reviewed: 2026-05-07.
