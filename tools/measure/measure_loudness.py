#!/usr/bin/env python3
"""measure_loudness.py
=====================

Measure integrated LUFS, sample peak, and true-peak (4x oversampled) for
every <slot>_bypass.wav / <slot>_wet.wav pair under
docs/validation/reaper/renders/, then print the deltas the V1 acceptance
gate needs (docs/spec/02-measurements.md sections 4 and 5).

Why this exists
---------------
The validation runbook lets you read LUFS off ReaLoudness Meter inside
REAPER, but reading numbers per slot per build by hand is slow. This
script reduces a full validation pass to one terminal command.

Usage
-----
    python tools/measure/measure_loudness.py                     # all slots
    python tools/measure/measure_loudness.py --slot E101         # one slot
    python tools/measure/measure_loudness.py --csv out.csv       # write CSV

Dependencies
------------
* Python 3.10+
* numpy
* pyloudnorm  (BS.1770-4 implementation)
* soundfile

Install once:
    pip install numpy pyloudnorm soundfile
"""

from __future__ import annotations

import argparse
import csv
import math
import os
import sys
from dataclasses import dataclass
from pathlib import Path
from typing import Iterable

try:
    import numpy as np
    import pyloudnorm as pyln
    import soundfile as sf
except ImportError as e:
    print(
        "Missing dependency: %s. Run `pip install numpy pyloudnorm soundfile`."
        % e.name,
        file=sys.stderr,
    )
    sys.exit(2)


@dataclass
class Measurement:
    """Bundle of values we extract per file."""

    slot: str
    kind: str          # "bypass" | "wet" | "stress"
    integrated_lufs: float
    sample_peak_dbfs: float
    true_peak_dbtp: float
    samplerate: int
    samples: int


def _hz_to_dbfs(linear: float) -> float:
    if linear <= 0.0:
        return -math.inf
    return 20.0 * math.log10(linear)


def true_peak_dbtp(samples: np.ndarray, samplerate: int) -> float:
    """Cheap 4x oversampled true-peak estimate.

    Not as accurate as a JUCE-style ITU 1770 true-peak meter but well
    within tolerance for spotting V1 ceiling violations.
    """
    if samples.ndim == 1:
        channels = [samples]
    else:
        channels = [samples[:, c] for c in range(samples.shape[1])]

    peak = 0.0
    for ch in channels:
        # FFT-based 4x upsample using zero-padding in the spectrum.
        n = len(ch)
        if n == 0:
            continue
        upsampled_n = n * 4
        spec = np.fft.rfft(ch)
        padded = np.zeros(upsampled_n // 2 + 1, dtype=spec.dtype)
        padded[: len(spec)] = spec
        upsampled = np.fft.irfft(padded, n=upsampled_n) * 4.0
        peak = max(peak, float(np.max(np.abs(upsampled))))

    return _hz_to_dbfs(peak)


def measure_file(path: Path, slot: str, kind: str) -> Measurement:
    samples, sr = sf.read(str(path), always_2d=False)
    if samples.ndim == 2 and samples.shape[1] > 2:
        samples = samples[:, :2]

    meter = pyln.Meter(sr)
    if samples.ndim == 1:
        loudness = meter.integrated_loudness(samples)
    else:
        loudness = meter.integrated_loudness(samples)

    sample_peak = float(np.max(np.abs(samples))) if samples.size else 0.0
    return Measurement(
        slot=slot,
        kind=kind,
        integrated_lufs=loudness,
        sample_peak_dbfs=_hz_to_dbfs(sample_peak),
        true_peak_dbtp=true_peak_dbtp(samples, sr),
        samplerate=sr,
        samples=samples.shape[0],
    )


def find_slots(renders_dir: Path) -> list[str]:
    return sorted(p.name for p in renders_dir.iterdir() if p.is_dir())


def gather_measurements(renders_dir: Path, slot_filter: str | None) -> Iterable[Measurement]:
    slots = find_slots(renders_dir)
    if slot_filter:
        slots = [s for s in slots if s == slot_filter]

    for slot in slots:
        slot_dir = renders_dir / slot
        for kind in ("bypass", "wet", "stress"):
            path = slot_dir / f"{slot}_{kind}.wav"
            if path.exists():
                yield measure_file(path, slot, kind)


def report(measurements: list[Measurement], csv_path: str | None) -> int:
    """Print human-readable table and a CSV if requested.

    Returns:
        Process exit code (0 = pass on inspection, 1 = at least one
        ceiling violation detected).
    """
    grouped: dict[str, dict[str, Measurement]] = {}
    for m in measurements:
        grouped.setdefault(m.slot, {})[m.kind] = m

    failures = 0
    print(f"\n{'slot':<6} {'kind':<7} {'LUFS':>8} {'peak dBFS':>10} {'TP dBTP':>10}  delta")
    print("-" * 60)

    for slot in sorted(grouped):
        bypass = grouped[slot].get("bypass")
        wet    = grouped[slot].get("wet")
        for kind in ("bypass", "wet", "stress"):
            m = grouped[slot].get(kind)
            if not m:
                continue
            delta = ""
            if kind == "wet" and bypass is not None:
                d_lufs = m.integrated_lufs - bypass.integrated_lufs
                d_tp   = m.true_peak_dbtp - bypass.true_peak_dbtp
                delta = f"  dLUFS={d_lufs:+.2f} dTP={d_tp:+.2f}"
                # Hard ceiling checks per docs/spec/02-measurements.md §4/§5.
                if d_lufs > 1.5:
                    delta += " [LUFS-DELTA-EXCEEDED]"
                    failures += 1
                if d_tp > 1.0:
                    delta += " [TP-DELTA-EXCEEDED]"
                    failures += 1
            if m.sample_peak_dbfs > -1.0 + 1.0e-3:
                delta += " [CEILING-VIOLATION]"
                failures += 1
            print(f"{m.slot:<6} {m.kind:<7} {m.integrated_lufs:>8.2f} "
                  f"{m.sample_peak_dbfs:>10.2f} {m.true_peak_dbtp:>10.2f}{delta}")

    if csv_path:
        with open(csv_path, "w", newline="", encoding="utf-8") as f:
            w = csv.writer(f)
            w.writerow(["slot", "kind", "integrated_lufs",
                        "sample_peak_dbfs", "true_peak_dbtp",
                        "samplerate", "samples"])
            for m in measurements:
                w.writerow([m.slot, m.kind, f"{m.integrated_lufs:.4f}",
                            f"{m.sample_peak_dbfs:.4f}",
                            f"{m.true_peak_dbtp:.4f}",
                            m.samplerate, m.samples])
        print(f"\nWrote CSV: {csv_path}")

    print(f"\nTotal failures: {failures}")
    return 0 if failures == 0 else 1


def main(argv: list[str]) -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                      formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--renders", default=None,
                        help="Path to docs/validation/reaper/renders. Defaults to repo-relative.")
    parser.add_argument("--slot", default=None, help="Restrict to a single slot, e.g. E101.")
    parser.add_argument("--csv",  default=None, help="Optional CSV output path.")
    args = parser.parse_args(argv)

    if args.renders:
        renders = Path(args.renders)
    else:
        repo_root = Path(__file__).resolve().parents[2]
        renders = repo_root / "docs" / "validation" / "reaper" / "renders"

    if not renders.exists():
        print(f"Renders directory not found: {renders}", file=sys.stderr)
        return 2

    measurements = list(gather_measurements(renders, args.slot))
    if not measurements:
        print("No render files found to measure. Run the REAPER batch first.",
              file=sys.stderr)
        return 2

    return report(measurements, args.csv)


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
