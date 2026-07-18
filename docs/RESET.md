# Reset verification

## Defined reset state

`Voice::reset()` restores every mutable part of the current DSP voice to its
configured start state without reconstructing the object:

- the pattern generator reloads the configured seed in 8/16 mode;
- 127 mode masks the configured seed to seven bits and maps zero to state 1;
- oscillator 1 returns to -2.5 V moving upward and oscillator 2 returns to
  +1.25 V moving downward;
- both state-variable-filter integrators return to zero;
- stepped CV is recomputed from the restored pattern and configured taps;
- internal-pulse and external-Schmitt clock edge history returns to its known
  initial state; and
- the bounded display status reports the restored oscillator and pattern state
  as inactive until the next processed frame.

The configured sample rate, parameters, quality, routes, taps, and seed remain
in effect. Reset changes running DSP state, not configuration.

## Retained host evidence

`tests/voice_reset_test.cpp` first disturbs oscillator, filter, pattern, stepped
CV, activity, and clock-edge state, then resets the same object. It compares
that object bit-for-bit with a freshly configured voice for 512 subsequent
frames. The matrix covers:

- 32, 44.1, 48, 88.2, and 96 kHz;
- Eco, Normal, and High quality;
- 8/16 and 127 modes;
- internal and external clock routes; and
- zero and non-zero configured seeds.

Those dimensions produce 120 reset contexts and 61,440 compared post-reset
frames. The test also checks the immediate documented oscillator voltages,
pattern state, configured seed, and inactive status. Every 127-mode frame is
checked to remain non-zero. Starting the external-clock render high makes a
stale Schmitt latch observable; the internal-clock setup similarly leaves the
oscillator-2 pulse detector high before reset.

The focused component check in `tests/pattern_generator_test.cpp` separately
proves that reset restores seed `0xa5` in 8/16 mode, maps a zero seed to state 1
in 127 mode, and remains outside zero lock for 512 direct recurrence shifts.

Run the retained evidence with:

```sh
make test
```

## Acceptance conclusion

The source reset paths and retained host tests complete AC-012: Reset restores
the configured known state and prevents zero lock in 127 mode. Hardware
recovery through the eventual plug-in Reset control remains part of AC-013,
not this host-DSP criterion.
