# Changelog

## Unreleased

- Extract the multimode state-variable filter into a directly tested,
  allocation-free DSP component.
- Correct the internal filter source to the V1 resistor-network estimate for
  PWM and previous stepped CV, and level-match the external-audio crossfade.
- Curve Resonance from Q 0.5 to Q 20 with positive damping, natural percussive
  ping decay, resonance input compensation, and controlled pole-frequency
  skew.
- Correct `Input drive` so 0.25x..4x remains transparent gain for ordinary
  source-scaled +/-5 V operation, with soft input protection kept out of range.
- Keep LP, BP, and HP DC-coupled; LP offsets and sub-audio movement remain
  available by design.
- Add direct and plug-in-level regression coverage for ping decay, silence
  stability, DC response, source routing, drive, harmonics, reset,
  determinism, and bounded output.
- Add deterministic pre/post listening WAVs, static/limiter analysis, and a
  five-rate 16x double-precision/511-tap-FIR numerical oracle to `make verify`.

This candidate retains the v1 `ThBu` factory identity and all 50 positional
parameters. A version number will be assigned only after physical-module
listening and processor-load acceptance.
