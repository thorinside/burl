#!/usr/bin/env python3
"""Measure deterministic Burl renders for broadband static and discontinuities."""

from __future__ import annotations

import argparse
import json
import math
import wave
from pathlib import Path

import numpy as np


def read_mono_wav(path: Path) -> tuple[int, np.ndarray]:
    with wave.open(str(path), "rb") as wav_file:
        if wav_file.getnchannels() != 1 or wav_file.getsampwidth() != 2:
            raise ValueError(f"{path} must be mono 16-bit PCM")
        sample_rate = wav_file.getframerate()
        samples = np.frombuffer(wav_file.readframes(wav_file.getnframes()), "<i2")
    return sample_rate, samples.astype(np.float64) / 32768.0


def db_ratio(numerator: float, denominator: float) -> float:
    if numerator <= 0.0:
        return -300.0
    return 10.0 * math.log10(numerator / max(denominator, 1.0e-300))


def analyze(path: Path) -> dict[str, float | int | str]:
    sample_rate, samples = read_mono_wav(path)
    analysis = samples[sample_rate // 4 :]
    window = np.hanning(analysis.size)
    spectrum = np.fft.rfft((analysis - np.mean(analysis)) * window)
    power = np.square(np.abs(spectrum))
    frequencies = np.fft.rfftfreq(analysis.size, 1.0 / sample_rate)
    audible = power[(frequencies >= 20.0) & (frequencies <= 20000.0)]
    high = power[(frequencies >= 5000.0) & (frequencies <= 20000.0)]
    low = power[(frequencies >= 20.0) & (frequencies < 5000.0)]
    difference = np.diff(analysis)
    absolute_difference = np.abs(difference)
    median_difference = float(np.median(absolute_difference))
    click_threshold = max(0.02, median_difference * 25.0)
    click_count = int(np.count_nonzero(absolute_difference > click_threshold))
    spectral_flatness = float(
        np.exp(np.mean(np.log(audible + 1.0e-300)))
        / max(np.mean(audible), 1.0e-300)
    )
    rms = float(np.sqrt(np.mean(np.square(analysis))))
    limiter_fraction = float(np.mean(np.abs(analysis) >= 0.8))
    return {
        "file": path.name,
        "sample_rate": sample_rate,
        "rms_dbfs": 20.0 * math.log10(max(rms, 1.0e-15)),
        "peak_dbfs": 20.0 * math.log10(max(float(np.max(np.abs(analysis))), 1.0e-15)),
        "peak_volts": 10.0 * float(np.max(np.abs(analysis))),
        "high_band_db": db_ratio(float(np.sum(high)), float(np.sum(low))),
        "spectral_flatness": spectral_flatness,
        "median_delta": median_difference,
        "max_delta": float(np.max(absolute_difference)),
        "click_threshold": click_threshold,
        "click_count": click_count,
        "limiter_fraction": limiter_fraction,
    }


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("directory", type=Path)
    parser.add_argument("--json", action="store_true")
    parser.add_argument("--json-output", type=Path)
    parser.add_argument("--assert-clean", action="store_true")
    parser.add_argument("--assert-eurorack-level", action="store_true")
    args = parser.parse_args()

    results = [analyze(path) for path in sorted(args.directory.glob("*.wav"))]
    if args.json_output is not None:
        args.json_output.write_text(
            json.dumps(results, indent=2, sort_keys=True) + "\n",
            encoding="utf-8",
        )
    if args.json:
        print(json.dumps(results, indent=2, sort_keys=True))
    else:
        print("file                              rms dBFS   >5k/low dB  flatness  limit %  clicks  max delta")
        for result in results:
            print(
                f"{result['file']:<34}"
                f"{result['rms_dbfs']:>9.2f}"
                f"{result['high_band_db']:>14.2f}"
                f"{result['spectral_flatness']:>10.5f}"
                f"{100.0 * result['limiter_fraction']:>9.3f}"
                f"{result['click_count']:>8d}"
                f"{result['max_delta']:>11.5f}"
            )

    if args.assert_clean:
        normal_outputs = [
            result
            for result in results
            if result["file"] in {
                "default_normal_lp.wav",
                "default_normal_bp.wav",
                "default_normal_hp.wav",
            }
        ]
        normal_lp = next(
            result for result in normal_outputs if result["file"] == "default_normal_lp.wav"
        )
        limiting = max(result["limiter_fraction"] for result in normal_outputs)
        if (
            normal_lp["high_band_db"] > -45.0
            or normal_lp["click_count"] > 0
            or limiting > 0.01
        ):
            print(
                "FAIL: default Normal filter render contains broadband static, "
                "discontinuities, or sustained limiter activity "
                f"(>5 kHz={normal_lp['high_band_db']:.2f} dB, "
                f"clicks={normal_lp['click_count']}, "
                f"max limiter occupancy={100.0 * limiting:.2f}%)"
            )
            return 1
    if args.assert_eurorack_level:
        normal_outputs = [
            result
            for result in results
            if result["file"] in {
                "default_normal_lp.wav",
                "default_normal_bp.wav",
                "default_normal_hp.wav",
            }
        ]
        out_of_range = [
            result
            for result in normal_outputs
            if result["peak_volts"] < 3.0 or result["peak_volts"] > 8.0
        ]
        if len(normal_outputs) != 3 or out_of_range:
            levels = ", ".join(
                f"{result['file']}={result['peak_volts']:.2f} V"
                for result in normal_outputs
            )
            print(
                "FAIL: default Normal filter outputs must reach Eurorack "
                f"level without entering the limiter knee ({levels})"
            )
            return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
