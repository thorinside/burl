# Changelog

## Unreleased

- Prevent non-finite public DSP parameters from poisoning oscillator state or
  reaching undefined resonance-table index conversion, with sanitizer-backed
  recovery coverage for all 16 floating voice parameters.
- Resolve output bus pointers and Add/Replace modes once per host block instead
  of repeating the same routing work for every sample.
- Clarify the filter character's pre-normalization voltage domain and the
  convergence-only scope and linear-range assumptions of the numerical oracle.

## 1.0.1 - 2026-07-19

- Extract the multimode state-variable filter into a directly tested,
  allocation-free DSP component.
- Correct the internal filter source to the V1 resistor-network estimate for
  PWM and previous stepped CV, and level-match the external-audio crossfade.
- Curve Resonance from Q 0.5 to Q 20 with positive damping, natural percussive
  ping decay, resonance input compensation, and controlled pole-frequency
  skew.
- Correct `Input drive` so 0.25x..4x remains transparent gain for ordinary
  source-scaled +/-5 V operation, with soft input protection kept out of range.
- Add 10x final filter-output normalization after the resonant core and before
  the oversampled limiter, restoring Eurorack-level LP/BP/HP signals without
  changing their character.
- Keep LP, BP, and HP DC-coupled; LP offsets and sub-audio movement remain
  available by design.
- Allow `None` (`0`) on all eight output routes while preserving their existing
  Outputs 1-8 defaults, positions, and Add/Replace modes.
- Add direct and plug-in-level regression coverage for ping decay, silence
  stability, DC response, source routing, drive, harmonics, reset,
  determinism, and bounded output.
- Add deterministic pre/post listening WAVs, static/limiter analysis, and a
  five-rate 16x double-precision/511-tap-FIR numerical oracle to `make verify`.

This release retains the v1 `ThBu` factory identity and all 50 positional
parameters. It completed physical-module listening and processor-load
acceptance before release.
