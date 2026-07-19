# Filter ping, drive, and character verification

## Signal path

The filter is an allocation-free topology-preserving state-variable filter in
`StateVariableFilter`. It exposes low-pass, band-pass, and high-pass values
from the same internal state and is processed at the voice's selected common
internal quality rate.

With no external audio route selected, the filter source is the bipolar PWM
comparator plus ten percent of the stepped-CV value from the start of that
internal step:

```text
internal source = PWM + 0.10 * previous stepped CV
```

The existing `Input mix` path crossfades linearly between that source and the
conditioned external audio input. `Input drive` then applies 0.25x through 4x
gain before the filter. Protection is exactly linear through +/-10 V after
drive and approaches +/-12 V progressively above that knee. Ordinary +/-5 V
operation at 1x drive is therefore unchanged; higher drive settings add a
bounded pre-filter saturation region instead of behaving as plain gain into a
hard output ceiling.

## Resonance and ping behavior

The normalized Resonance value `r` maps to:

```text
Q = 0.5 * 40^r
damping = 1 / Q
```

A 65-entry damping table with linear interpolation implements the curve. It
runs from Q 0.5 to Q 20 and always supplies positive damping, so the filter can
ring strongly after an edge without starting or sustaining an oscillator from
silence.

The driven input is compensated by `1 - 0.25 * r^2`. Resonant waveform skew is
derived from the previous first-pole state and added to the cutoff modulation
before the existing cutoff calculation:

```text
skew octaves = 0.5 * r^2 * clamp(previous band / 5, -1, 1)
```

That asymmetric frequency movement supplies controlled even and odd harmonic
content at high resonance. It does not add another oscillator or a separate
character control.

At 96 kHz, the retained 250 Hz impulse fixture measures the final sample above
-60 dB at approximately 42.5 ms for 62% resonance and 174.3 ms for maximum
resonance. A maximum-resonance filter remains exactly silent without an
excitation and decays below 1 microvolt after excitation.

## DC behavior

LP, BP, and HP are deliberately DC-coupled. There is no blocker, hidden
high-pass stage, or DC-block parameter.

The LP output passes steady and slowly changing offsets by design. For an ideal
steady input, BP and HP naturally settle toward zero, but their transient and
sub-audio behavior is retained. Patch a downstream high-pass or AC-coupling
utility when a destination should not receive the LP offset.

## Output protection

The optional `Output limit` remains inside the common oversampled filter path.
It is exactly linear through +/-8 V and approaches +/-10 V progressively.
Finite emergency bounds protect the filter state and outputs from invalid or
extreme inputs without changing ordinary modular-level operation.

## Retained verification

`tests/filter_test.cpp` directly verifies the Q curve, both impulse tails,
silence stability, post-excitation decay, DC behavior, bounded drive response,
and high-resonance even/odd harmonic generation.

`tests/voice_source_routing_test.cpp` compares the internal-source render
against a second voice receiving `PWM + 0.10 * previous stepped CV` through its
external input and requires bit-exact LP, BP, and HP output.

`tests/plugin_integration_test.cpp` exercises `Input drive` through the actual
NT parameter definition and callback. It verifies the frozen 25..400 host
range and 0.25x..4.00x display, low-level gain ratios, and progressive
saturation at normal input levels.

Run the full retained verification with:

```sh
make verify
```

The host and native checks are necessary but do not replace the pending owner
listening comparison on the physical module for this unreleased candidate.
