#!/usr/bin/env python3
"""Compare Burl quality modes with the 16x double-precision filter oracle."""

from __future__ import annotations

import argparse
import math
import wave
from pathlib import Path

import numpy as np


def read_wav(path: Path) -> tuple[int, np.ndarray]:
    with wave.open(str(path), "rb") as wav_file:
        if wav_file.getnchannels() != 1 or wav_file.getsampwidth() != 3:
            raise ValueError(f"{path} must be mono 24-bit PCM")
        rate = wav_file.getframerate()
        raw = np.frombuffer(
            wav_file.readframes(wav_file.getnframes()), dtype=np.uint8
        ).reshape(-1, 3)
    samples = (
        raw[:, 0].astype(np.int32)
        | (raw[:, 1].astype(np.int32) << 8)
        | (raw[:, 2].astype(np.int32) << 16)
    )
    samples = np.where(samples & 0x800000, samples - 0x1000000, samples)
    return rate, samples.astype(np.float64) / 8388608.0


def spectrum_error(reference: np.ndarray, candidate: np.ndarray) -> float:
    window = np.blackman(reference.size)
    reference_magnitude = np.abs(np.fft.rfft(reference * window))
    candidate_magnitude = np.abs(np.fft.rfft(candidate * window))
    numerator = np.linalg.norm(candidate_magnitude - reference_magnitude)
    denominator = max(np.linalg.norm(reference_magnitude), 1.0e-300)
    return 20.0 * math.log10(max(numerator / denominator, 1.0e-15))


def high_band_delta(reference: np.ndarray, candidate: np.ndarray,
                    sample_rate: int) -> float:
    window = np.blackman(reference.size)
    frequencies = np.fft.rfftfreq(reference.size, 1.0 / sample_rate)
    selection = (frequencies >= 5000.0) & (frequencies <= 20000.0)
    reference_power = np.sum(np.abs(np.fft.rfft(reference * window))[selection] ** 2)
    candidate_power = np.sum(np.abs(np.fft.rfft(candidate * window))[selection] ** 2)
    return 10.0 * math.log10(max(candidate_power, 1.0e-300) / max(reference_power, 1.0e-300))


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("directory", type=Path)
    parser.add_argument("--assert-converges", action="store_true")
    args = parser.parse_args()

    rows: list[tuple[int, str, str, str, float, float]] = []
    for rate in (32000, 44100, 48000, 88200, 96000):
        for stimulus in ("sine", "square"):
            for output in ("lp", "bp", "hp"):
                reference_rate, reference = read_wav(
                    args.directory
                    / f"sr{rate}_reference16x_{stimulus}_{output}.wav"
                )
                if reference_rate != rate:
                    raise ValueError("reference WAV has the wrong sample rate")
                start = rate // 4
                reference = reference[start:]
                for quality in ("eco", "normal", "high"):
                    candidate_rate, candidate = read_wav(
                        args.directory
                        / f"sr{rate}_production_{quality}_{stimulus}_{output}.wav"
                    )
                    if candidate_rate != rate:
                        raise ValueError("reference and candidate sample rates differ")
                    candidate = candidate[start:]
                    rows.append(
                        (
                            rate,
                            stimulus,
                            output,
                            quality,
                            spectrum_error(reference, candidate),
                            high_band_delta(reference, candidate, rate),
                        )
                    )

    print("rate  stimulus output quality  spectrum error dB  high-band delta dB")
    for rate, stimulus, output, quality, error, high_delta in rows:
        print(
            f"{rate:<5} {stimulus:<8} {output:<6} {quality:<7}"
            f"{error:>18.2f}{high_delta:>20.2f}"
        )

    if args.assert_converges:
        failures: list[str] = []
        for rate in (32000, 44100, 48000, 88200, 96000):
            for stimulus in ("sine", "square"):
                for output in ("lp", "bp", "hp"):
                    selected = [
                        row
                        for row in rows
                        if row[0] == rate
                        and row[1] == stimulus
                        and row[2] == output
                    ]
                    errors = {row[3]: row[4] for row in selected}
                    # Square-wave convergence exercises the discontinuous
                    # source that exposed the static bug. Pure-sine residuals
                    # eventually become limited by the production float core
                    # at very high internal rates, so gate those by an absolute
                    # -65 dB error floor instead of demanding monotonicity.
                    if (
                        stimulus == "square"
                        and errors["high"] > errors["eco"] + 0.25
                    ):
                        failures.append(
                            f"{rate} {stimulus}/{output} High does not converge beyond Eco"
                        )
                    if stimulus == "sine" and errors["high"] > -65.0:
                        failures.append(
                            f"{rate} {stimulus}/{output} High error is only "
                            f"{errors['high']:.2f} dB"
                        )
                    high_delta = next(
                        row[5] for row in selected if row[3] == "high"
                    )
                    if stimulus == "square" and abs(high_delta) > 3.0:
                        failures.append(
                            f"{rate} {stimulus}/{output} High high-band delta is "
                            f"{high_delta:.2f} dB"
                        )
        if failures:
            for failure in failures:
                print(f"FAIL: {failure}")
            return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
