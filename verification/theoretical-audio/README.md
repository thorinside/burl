# Retained theoretical WAV set

These files are deterministic code renders, not physical-instrument samples.
They use +/-10 V as WAV full scale.

- `v1.0.0/` is the released baseline rendered from `main` (`15bafbe`).
- `pre-fix/` is the first filter-character candidate (`fb08822`), which
  reproduces the reported static-like limiter behavior.
- `corrected-candidate/` is the source-scaled V1 candidate. Its normalized BP
  ping files are listening aids and must not be used as voltage evidence.
- `reference-48k/` is the 48 kHz subset of the 16x double-precision,
  511-tap-FIR oracle. These three files are mono 24-bit PCM; the matched voice
  renders are mono 16-bit PCM.
- Each `metrics.json` was produced by
  `scripts/analyze_theoretical_audio.py`; analysis skips the first 250 ms.

Regenerate the complete ignored matrices with:

```sh
make theoretical-audio-check
make filter-reference-check
```

The diagnosis, equations, metrics, and limitations are recorded in
`docs/THEORETICAL_AUDIO_VERIFICATION.md`.
