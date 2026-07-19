# Changelog

## Unreleased

- Extract the multimode state-variable filter into a directly tested,
  allocation-free DSP component.
- Correct the internal filter source to bipolar PWM plus ten percent of the
  previous stepped-CV value while preserving the external-audio crossfade.
- Curve Resonance from Q 0.5 to Q 20 with positive damping, natural percussive
  ping decay, resonance input compensation, and controlled pole-frequency
  skew.
- Correct `Input drive` so 0.25x..4x is transparent gain at low levels and
  progressive pre-filter saturation above the normal linear region.
- Keep LP, BP, and HP DC-coupled; LP offsets and sub-audio movement remain
  available by design.
- Add direct and plug-in-level regression coverage for ping decay, silence
  stability, DC response, source routing, drive, harmonics, reset,
  determinism, and bounded output.

This candidate retains the v1 `ThBu` factory identity and all 50 positional
parameters. A version number will be assigned only after physical-module
listening and processor-load acceptance.
