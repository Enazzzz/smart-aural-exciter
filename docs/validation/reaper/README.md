# REAPER validation projects

This folder holds the REAPER project file used to run the V1 validation suite.

Expected file:

- `v1-validation.RPP` - reference REAPER project per
  `../04-validation-suite.md` section 2.

Project layout (mirror this when creating it the first time):

```
Track 1: REF       (load one corpus slot at a time)
Track 2: BYPASS    (rendered bounce, plugin internally bypassed)
Track 3: WET       (rendered bounce, plugin enabled, default preset)
Master:  Smart Exciter (default preset, advanced panel hidden)
         ITU-R BS.1770-4 LUFS / true-peak meter (last in chain)
```

Project settings:

- 48 kHz, 256 sample buffer, 32-bit float renders.
- Stereo master, pre-fader metering, peak + RMS visible.
- Host-level dither disabled for offline renders.

Renders should go to a per-slot subfolder (`renders/<slot>/`) under this
directory but those folders are .gitignored - only the `.RPP` and notes
are committed.
