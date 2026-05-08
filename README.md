# Smart Exciter (working title)

A Windows VST3 smart exciter aimed at the master bus.
Goal: add clarity and detail to a finished mix without harshness or listener fatigue,
using a perceptual model that places harmonics only where the ear actually expects them.

## Status

V1 specification phase. Implementation not started yet.

## Repository layout

```
.cursor/                Cursor project configuration directory (rules, commands, etc.)
docs/
  spec/                 Locked V1 specification documents
    01-scope-freeze.md          Final scope decisions, in/out of V1
    02-measurements.md          Numeric thresholds (CPU, latency, loudness, safety)
    03-dsp-architecture.md      Detailed DSP block architecture
  validation/
    04-validation-suite.md      Reference audio corpus and A/B workflow in REAPER
```

The original V1 spec lives outside this repo at
`c:\Users\Zanea\.cursor\plans\smart-exciter-v1-spec_65b2f8c9.plan.md` and is the
upstream source of truth. The documents under `docs/spec/` and `docs/validation/`
expand and lock that spec into actionable artifacts.

## Top-level decisions (locked)

- Format: VST3 only.
- Platform: Windows only.
- Primary insertion point: stereo master bus.
- Default sonic identity: transparent.
- Quality bias: high quality first; latency budget allowed.
- Calibration target: electronic-first (Aphex-style detail/transients), rock-compatible.
- Validation audience: personal use first, occasional non-audiophile checks.
- License model: free in V1, paid/pro path later.
- Copy protection in V1: none.
