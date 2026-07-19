# Filter ping, drive, and character verification

## Signal path

The filter is an allocation-free topology-preserving state-variable filter in
`StateVariableFilter`. It exposes low-pass, band-pass, and high-pass values
from the same internal state and is processed at the voice's selected common
internal quality rate.

The internal source follows the zero-state forcing derived from the published
V1 input resistor network, converted from the documented native source ranges
to Burl's +/-5 V digital PWM and stepped CV:

```text
internal forcing = 0.021872 * PWM
                 + 0.044231 * previous stepped CV
```

For level-matched `Input mix` behavior, the weighted internal source is first
normalized to +/-5 V. It crossfades linearly with the conditioned external
audio and then receives the common `0.066103` forcing gain. A nominal +/-5 V
external input therefore reaches the filter summer at up to +/-0.330515 V,
instead of jumping roughly fifteen-fold above the internal endpoint.

`Input drive` then applies 0.25x through 4x gain before the filter. Protection
is exactly linear through +/-10 V at the driven filter input and approaches
+/-12 V progressively above that knee. Because the source-scaled voice path
cannot reach that knee from an ordinary +/-5 V input even at 4x, `Input drive`
is transparent gain throughout its supported host range. The soft bound is
retained as emergency protection, not as an ordinary waveshaper.

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

The driven input is compensated by `1 - 0.25 * r^2`. The fixed V1 R39 path is
represented by the previous first-pole state and added to the cutoff modulation
before the existing cutoff calculation:

```text
skew octaves = 0.5 * previous band volts
```

The path is neither resonance-gated nor normalized/clamped. Its audible depth
grows naturally as the first-pole amplitude grows with resonance. That
asymmetric frequency movement supplies controlled even and odd harmonic
content without adding another oscillator or a separate character control.

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

The core LP, BP, and HP values receive fixed 10x final normalization after the
filter state and character feedback have been updated. This restores Eurorack
level without changing the resonant loop: the default Normal fixture peaks at
approximately 5.19 V LP, 3.60 V BP, and 4.59 V HP.

The optional `Output limit` follows that normalization inside the common
oversampled filter path. It is exactly linear through +/-8 V and approaches
+/-10 V progressively. The default fixture has 0% limiter occupancy; 2x Input
drive begins to touch the LP limiter, while 4x intentionally drives it harder.
Finite emergency bounds protect the filter state and outputs from invalid or
extreme inputs.

## Retained verification

`tests/filter_test.cpp` directly verifies the Q curve, both impulse tails,
silence stability, post-excitation decay, DC behavior, bounded drive response,
high-resonance even/odd harmonic generation, final output gain, limiter order,
and proof that output normalization cannot feed back into the core state.

`tests/voice_source_routing_test.cpp` compares the internal-source render
against a second voice receiving the normalized V1 PWM/RUNCV forcing through
its external input, with a ten-microvolt calibrated-output tolerance equivalent
to one microvolt at the unnormalized filter core. It also requires nominal
+/-5 V external square audio at 1x drive to remain out of the limiter.

`tests/plugin_integration_test.cpp` exercises `Input drive` through the actual
NT parameter definition and callback. It verifies the frozen 25..400 host
range and 0.25x..4.00x display, low-level gain ratios, and progressive
protection outside normal input levels. It also requires 2x and 4x to remain
linear at a nominal +/-5 V external input.

Run the full retained verification with:

```sh
make verify
```

The code-generated listening fixtures, pre/post static metrics, and five-rate
16x oracle are documented in
[`THEORETICAL_AUDIO_VERIFICATION.md`](THEORETICAL_AUDIO_VERIFICATION.md).

The owner accepted the final loaded object by direct listening on the physical
module on 2026-07-19. That hardware judgment remains the sound-quality oracle;
the retained fixtures establish repeatability and regression coverage.
