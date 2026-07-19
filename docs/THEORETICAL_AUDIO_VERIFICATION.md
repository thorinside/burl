# Theoretical audio and static-bug verification

## Outcome

The static-like sound in the first filter-character candidate was reproduced
without hardware, reduced to a filter-source level error, and corrected. The
candidate had fed modular-level PWM almost directly into a high-Q digital SVF.
At the default 250 Hz cutoff and 62% resonance, the resulting six-second Normal
render spent 18.69% of LP samples, 3.82% of BP samples, and 5.04% of HP samples
inside the output limiter. The corrected V1-network render spends 0% there.

This result is code-generated evidence, not a recording or fit from a physical
instrument. It proves that the implementation no longer manufactures the
reported limiter/static fault and that its numerical filter converges to the
documented candidate equations. Physical listening remains the acceptance test
for whether those equations capture the desired instrument character.

## Root cause and correction

The published 2009 resistor network gives the following zero-state forcing at
the high-pass summer:

```text
h source = -0.01367 * native PWM - 0.04021 * native RUNCV
```

Using the documented approximate native ranges, Burl's +/-5 V digital sources
therefore use:

```text
PWM forcing     = 0.021872 * PWM
stepped forcing = 0.044231 * previous stepped CV
maximum same-sign forcing = +/-0.330515 V
```

The pre-fix candidate instead used `PWM + 0.10 * previous stepped CV`, reaching
about +/-5.5 V before drive. That was roughly 17 times the worst-case composite
forcing and about 46 times the initial PWM forcing implied by the V1 network.
Removing the character skew did not materially remove the LP limiting; lowering
resonance or input gain did. This localized the fault to excessive excitation,
not spontaneous filter noise or the fixed character path.

The external side of `Input mix` had the same mismatch. Internal audio had been
corrected to V1 summer scale while external audio still entered at raw +/-5 V,
making a full crossfade act like an approximately 15-fold gain jump. Both
endpoints now crossfade in a normalized +/-5 V domain and then receive the
common `0.066103` V1 forcing gain. `Input drive` remains a transparent 0.25x to
4x multiplier after that level-matched crossfade.

## Deterministic listening renders

`tools/render_theoretical_audio.cpp` executes the actual `Voice` implementation
and writes deterministic 48 kHz WAV files. `scripts/analyze_theoretical_audio.py`
measures level, high-band energy, spectral flatness, discontinuities, and
limiter occupancy. `make theoretical-audio-check` fails if the default Normal
LP contains a broadband floor or discontinuities, if any default Normal filter
output spends more than 1% of the render in the limiter, or if any of LP/BP/HP
fails to reach 3 V before the 8 V limiter knee.

The matched retained set is in `verification/theoretical-audio`:

| Render | LP RMS | BP RMS | HP RMS | Maximum limiter occupancy |
|---|---:|---:|---:|---:|
| v1.0.0 baseline | -13.49 dBFS | -25.98 dBFS | -33.72 dBFS | 0% |
| first character candidate | -5.22 dBFS | -8.12 dBFS | -7.93 dBFS | 18.69% |
| corrected V1 candidate | -15.10 dBFS | -20.57 dBFS | -20.34 dBFS | 0% |

The source correction deliberately lowered the internal resonant-core drive.
Owner listening then confirmed that the character sounded right but identified
the unnormalized 0.36-0.52 V peaks as line-level rather than Eurorack-level.
A fixed 10x stage now follows the core state update and precedes the limiter.
The same default patch reaches 5.19 V LP, 3.60 V BP, and 4.59 V HP with 0%
limiter occupancy. A retained state-equivalence test proves that these outputs
are exactly 10 times the unnormalized outputs and cannot alter the filter loop.

The retained `ping_res62_bp_audition.wav` and
`ping_res100_bp_audition.wav` files have a half-second lead-in and are peak
normalized for listening. They demonstrate decay shape only; they are not
voltage-level evidence. The direct unit test measures the unnormalized 250 Hz
-60 dB tails at approximately 42.5 ms and 174.3 ms.

## Independent high-rate numerical oracle

`tools/render_filter_reference.cpp` independently expresses the candidate SVF
in double precision. For every supported delivery rate (32, 44.1, 48, 88.2,
and 96 kHz), it:

1. renders at 16 times the delivery rate;
2. applies a 511-tap Blackman-windowed sinc FIR;
3. uses a cutoff of the lesser of 20 kHz and 45% of delivery rate;
4. decimates to the delivery rate; and
5. writes mono 24-bit PCM reference WAVs.

The oracle includes the candidate's analytic Q curve, quadratic input
compensation, and fixed 0.5-octave-per-volt first-pole feedback. It deliberately
compares the unnormalized core and omits the production input soft bound and
finite state clamps because the retained stimuli stay inside their linear
operating range. The separate actual-Voice render gates final output
normalization, protection, and limiter order. It is therefore a numerical
convergence oracle for the candidate equations, not independent proof that
every one of those equations is the unique analog-circuit solution.

`scripts/compare_filter_reference.py` compares Eco, Normal, and High production
renders with the oracle by in-band magnitude-spectrum residual and high-band
energy. For the discontinuous 53 Hz square source, LP/BP residuals improve by
roughly 6 to 7 dB per quality step at every delivery rate:

| Rate | Normal LP/BP error | High LP/BP error | High HP error |
|---:|---:|---:|---:|
| 32 kHz | -46.20 / -43.54 dB | -53.32 / -50.68 dB | -34.57 dB |
| 44.1 kHz | -48.77 / -45.83 dB | -55.29 / -52.33 dB | -35.97 dB |
| 48 kHz | -50.18 / -47.69 dB | -56.54 / -53.83 dB | -35.62 dB |
| 88.2 kHz | -54.84 / -52.02 dB | -61.68 / -58.83 dB | -32.63 dB |
| 96 kHz | -56.02 / -53.45 dB | -62.48 / -59.61 dB | -32.34 dB |

High-mode square-wave high-band energy remains within 0.65 dB of the reference
across every rate and output. Sine-spectrum error is -67 dB or lower throughout
the complete matrix. At 88.2 and 96 kHz, Normal can beat High on that already
tiny sine residual because the 4x production core reaches single-precision
limits; the absolute High error remains at or below -71 dB.

Run both retained checks with:

```sh
make theoretical-audio-check
make filter-reference-check
```

Both are included in `make verify`.

## What remains unproven

- The published sources do not specify Burl's exact Q curve or the current
  quadratic input-compensation curve.
- `Input drive`, the input and output soft bounds, and their curves are Burl
  safety/performance features, not documented V1 circuit controls.
- The fixed 10x output stage is a Burl calibration to the documented +/-5 V
  Eurorack range and owner level feedback, not a claimed V1 component gain.
- The fixed character coefficient is component-derived, but the one-sample
  digital delay and full nonlinear loop still need listening comparison.
- The retained 48 kHz listening renders are 16-bit convenience artifacts; the
  numerical oracle uses 24-bit files and double internal state.
- No hardware recording was used to tune this correction. The owner completed
  and accepted direct physical-module listening on 2026-07-19; the retained
  matched WAVs remain code-generated evidence rather than hardware captures.
- LP, BP, and HP remain DC-coupled. The LP offset and sub-audio response are
  intentional; downstream AC coupling is the user's choice.
