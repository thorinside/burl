# Safety-limited voice stress verification

## Retained matrix and workload

`tests/voice_stress_test.cpp` exercises the integrated voice at every supported
sample rate (32, 44.1, 48, 88.2, and 96 kHz) and in every quality mode (Eco,
Normal, and High). Two distinct seeds produce 30 contexts. Each context runs
32,768 host frames, for 983,040 host frames and 7,864,320 checked output values
in total.

Each context moves through eight 4,096-frame parameter phases without
reconstructing the voice. The phases cover:

- oscillator rates from 0.001 Hz through values above the internal clamp;
- positive, negative, zero, and extreme oscillator and filter feedback;
- both feedback shift-register modes, Change endpoints and intermediate values,
  internal and external clocks, and single- and double-edge clocking;
- filter cutoff extremes, minimum and maximum resonance, positive and negative
  cutoff modulation, the full 0.25x..4x drive range, and the full
  internal/external input-mix range;
- internal oscillator normals and external oscillator-CV replacement routes;
  and
- all eight selectable register tap positions.

The deterministic input streams repeatedly reach the conditioned Eurorack
boundaries: +/-10 V oscillator and cutoff CV, +/-12 V filter audio, and a
Schmitt-clock sequence spanning -5 V, 0 V, and +5 V.

## Assertions

Safety limiting remains enabled throughout the matrix. On every host frame,
the test requires each of LP, BP, HP, stepped CV, PWM, register XOR, Aux A, and
Aux B to be finite and within the inclusive -10 V to +10 V safety bound. It
also requires both oscillator status values to remain finite and within their
-5 V to +5 V triangle bounds and confirms that processing reports the voice as
active.

The implementation supports these checks by conditioning routed external
values with `finiteClamp()`, clamping oscillator and filter operating ranges,
bounding the filter integrator states, replacing non-finite final values, and
applying transparent pre-filter protection and the selected soft output
limiter at the common internal quality rate before final finite bounds.

`make stress-sanitize` compiles and runs the same complete stress matrix with
AddressSanitizer and UndefinedBehaviorSanitizer. This retained instrumentation
checks the exercised paths for out-of-bounds and use-after-lifetime memory
accesses and undefined behavior in addition to the explicit numerical
assertions. Apple Clang's AddressSanitizer does not support leak detection on
this platform, so that unrelated option is disabled.

Run the normal and instrumented verification with:

```sh
make test
make stress-sanitize
```

## Acceptance conclusion

The complete matrix passes without a non-finite value, an output outside the
configured safety bound, an unbounded oscillator state, a sanitizer finding,
or an undefined-behavior diagnostic. This retained host evidence completes
AC-011 for the integrated Burl DSP voice at all supported sample rates and
quality settings.
