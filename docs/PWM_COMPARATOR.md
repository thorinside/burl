# PWM comparator verification

## Implementation evidence

`Voice::processInternal()` computes the PWM output directly from the two
newly advanced triangle values:

```text
OSC2 triangle > OSC1 triangle ? +5 V : -5 V
```

Equality therefore selects -5 V. This output is independent of
`registerXor`, which is calculated separately from pattern-generator bits.
No oscillator pulse XOR participates in the PWM calculation.

Eco mode returns the single internal comparison unchanged. Normal and High
apply the documented low-pass decimation to two or four internal bipolar
comparisons, so a host-rate frame that contains a transition may contain the
corresponding average.

## Retained test evidence

`testPwmIsBipolarTriangleComparator()` in `tests/voice_pwm_test.cpp` runs the
voice in Eco mode at binary-exact oscillator and sample rates. Oscillator 1
increases while oscillator 2 decreases, with neither direction changing, and
the interval covers all three triangle relationships: greater than, equality,
and less than.

For every frame the test requires the PWM output to equal the specified
comparison with zero tolerance and to be exactly +5 V or -5 V. It also requires
PWM to change once as the triangles cross even though both oscillator pulse
directions remain fixed. A pulse-direction XOR would remain constant over this
interval and cannot pass the test.

Run the retained verification with:

```sh
make test
```

## Acceptance conclusion

The direct implementation and focused host regression test complete AC-009:
PWM is the defined bipolar comparison of oscillator triangle values and is not
pulse XOR.
