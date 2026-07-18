# Stepped-CV verification

## Implementation evidence

`PatternGenerator::dac()` reads three selected register taps with binary weights
4, 2, and 1. The resulting three-bit code therefore has exactly the finite
domain 0 through 7. `PatternGenerator::dacLevel()` maps that domain with:

```text
volts = -5 + code * (10 / 7)
```

This gives eight ordered code values. Code 0 maps to exactly -5 V and code 7
to exactly +5 V in the C++11 implementation.

The integrated `Voice` initializes and updates its stepped CV only through this
mapping. `processInternal()` exposes the value before the next register clock
updates it, preserving the specified deterministic update order. Host-rate
quality decimation may average a transition between adjacent raw levels; that
is an allowed resampling effect and does not alter the pre-decimation mapping.

## Retained test evidence

`testDacHasEightExactMonotonicLevels()` in
`tests/pattern_generator_test.cpp` exhaustively checks all eight valid codes. It
proves that:

- code 0 equals -5 V with zero tolerance;
- every successive code is strictly greater than its predecessor, which makes
  all eight values unique and monotonic;
- code 7 equals +5 V with zero tolerance; and
- all eight combinations of taps 2, 1, and 0 reproduce their corresponding
  weighted code level with zero tolerance.

Run the retained verification with:

```sh
make test
```

## Acceptance conclusion

The implementation and exhaustive host test complete AC-008: before allowed
quality-rate resampling effects, the feedback shift-register pattern generator
produces exactly eight unique monotonic stepped-CV levels spanning exactly
-5 V to +5 V.
