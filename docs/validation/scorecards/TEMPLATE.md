# Validation scorecard - <SLOT>

Copy this file to `<SLOT>.md` (e.g. `E101.md`) for each slot under test.
See `../04-validation-suite.md` section 6 for the canonical template and
section 5 for the V1 acceptance gate this scorecard rolls up into.

- Slot:    <e.g. E101>
- File:    <filename from corpus.txt>
- Date:    YYYY-MM-DD
- Build:   <git sha or build label>
- Tester:  <name / initials>

## CPU / latency

|                 | Standard | High |
| --------------- | -------- | ---- |
| CPU % (60s avg) |          |      |
| Latency (samp)  |          |      |
| Null test       | PASS / FAIL |   |

## Loudness / true-peak

|                          | bypass | wet | delta |
| ------------------------ | ------ | --- | ----- |
| Integrated LUFS          |        |     |       |
| Max true-peak (dBTP)     |        |     |       |

## Safety stress (+12 dB pre-gain)

- Sample peak: ____ dBFS  (must be <= -1.0 dBFS)
- True-peak:   ____ dBTP  (must be within budget in spec/02-measurements.md)

## A/B (blind-ish, level matched within ~0.3 dB)

|                | Monitors | Headphones | Consumer |
| -------------- | -------- | ---------- | -------- |
| Preference     |          |            |          |
| One-line note  |          |            |          |

Allowed preference values: `PREFER_WET`, `TIE`, `PREFER_BYPASS`.

## Long-pass fatigue (only required once per release candidate)

- 20-minute mixed-program result: `OK` / `MILD` / `BAD`
- Notes:

## Non-audiophile spot check (optional, informational)

- Listener:
- Slot(s) tried:
- Preference notes:

## Overall result for this slot

- [ ] PASS
- [ ] FAIL (regression)
- [ ] SKIP (file missing / N/A)

## Notes

(observations, retune candidates, regressions vs prior build, anything
worth flagging in the next iteration)
