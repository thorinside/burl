# Determinism verification

## Current retained evidence

The host test `tests/determinism_test.cpp` renders the implemented feedback
shift-register pattern-generator slice twice from independently constructed
instances. Every pair has identical:

- configured seed;
- measured-interval initial state, established by the same 47-clock pre-roll;
- mode, Change values, and tap selections;
- chaos-signal input stream;
- sample-rate context; and
- quality-mode context.

The matrix covers 32, 44.1, 48, 88.2, and 96 kHz; Eco, Normal, and High; and
both pattern-generator modes. Each case compares 1,024 output frames containing
the complete register state, a selected register bit, and the three-tap DAC
output.

The retained tolerance for this discrete DSP slice is **zero**: integer and
boolean outputs must match exactly, and floating-point DAC outputs must have
identical 32-bit representations. The test rejects an empty render so a
vacuous comparison cannot pass.

The pattern generator itself is intentionally independent of sample rate and
quality. Those values identify the render context and vary the deterministic
test trajectories; actual sample-rate conversion and quality-rate processing
belong to the later integrated voice.

Run the retained check with:

```sh
make test
```

## Acceptance scope

This evidence proves deterministic repeated rendering for all DSP currently
implemented. AC-004 remains partial at the product level until the integrated
oscillators, filter, routing/mixing, and runtime quality processing use this
same independent-render comparison. At that point the complete output set must
retain either exact equality or an explicitly documented floating-point
tolerance.
