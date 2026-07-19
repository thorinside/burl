# Determinism verification

## Integrated voice evidence

The host test `tests/voice_determinism_test.cpp` renders the complete current
Burl DSP voice twice from independently constructed instances. Every pair has
identical:

- configured seed;
- measured-interval initial state, established by the same 257-frame pre-roll;
- oscillator, pattern-generator, filter, routing-normal, tap, and quality
  parameter values, including the extracted filter state, curved resonance,
  PWM/stepped-CV source, drive conditioning, and the same parameter change
  halfway through;
- oscillator CV, external clock, filter audio, and cutoff CV input streams;
- runtime sample rate; and
- quality mode.

The matrix covers 32, 44.1, 48, 88.2, and 96 kHz; Eco, Normal, and High; and
both internal-normal/internal-clock and external-replacement/external-clock
scenarios. Each case compares 2,048 frames containing all eight product
outputs, both oscillator status values, the complete register state, and the
activity flag. A non-constant-output check prevents a vacuous silent or fixed
render from passing.

The retained tolerance is **zero**. Every floating-point output and status
value must have an identical 32-bit representation, and integer and boolean
state must match exactly. This is stricter than the acceptance criterion's
allowance for a documented floating-point tolerance.

The voice consumes the sample rate at runtime. Eco, Normal, and High execute at
1x, 2x, and 4x common internal rates respectively, with scalar storage
preallocated in the voice and boxcar low-pass decimation to host rate. The test
therefore exercises actual rate and quality behavior rather than treating
those values only as metadata.

## Pattern-generator component evidence

The narrower host test `tests/determinism_test.cpp` independently renders the
feedback shift-register pattern generator across the same five sample-rate and
three quality contexts, both register modes, a 47-clock pre-roll, and 1,024
output frames. It compares complete register state, a selected bit, and DAC
float bits exactly. The pattern generator is intentionally rate-independent;
this component test remains as a focused regression check beneath the
integrated voice test.

## Acceptance conclusion

Together these retained tests complete AC-004 for the host DSP implementation:
identical seed, initial state, parameters, inputs, sample rate, and quality
produce bit-exact repeated renders. The native adapter uses this core and must
not bypass this regression gate; native loading, routing,
preset, and hardware behavior are covered by their separate acceptance
criteria.

Run all retained checks with:

```sh
make test
```
