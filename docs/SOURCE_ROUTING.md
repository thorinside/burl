# Internal-normal source replacement verification

## Implementation evidence

`Voice::processInternal()` chooses exactly one modulation source for each
oscillator. With no external route selected, oscillator 1 receives oscillator
2's triangle and oscillator 2 receives oscillator 1's triangle. Selecting an
external oscillator route chooses the corresponding conditioned external CV
instead. The selected values enter the logarithmic frequency calculation once;
the external CV is not added to the internal normal.

`Voice::clockEdge()` likewise has disjoint clock paths. With the internal route
selected, only oscillator 2's bipolar pulse is edge-detected. With the external
route selected, only the conditioned external Schmitt state is edge-detected.
The unselected source cannot clock the pattern generator.

The filter has a similarly explicit internal source. With the external-audio
crossfade fully internal, the V1 resistor-network estimate supplies
`0.021872 * PWM + 0.044231 * previous stepped CV`. Both internal and external
sources crossfade in a normalized +/-5 V domain before the common `0.066103`
filter-summer forcing gain, so selecting external audio does not introduce a
large gain discontinuity.

## Retained test evidence

`tests/voice_source_routing_test.cpp` verifies both oscillator CV routes and the
clock route in Eco mode so that each host frame is one observable internal
step:

- opposite external CV values produce bit-exact matching oscillator states
  while the internal normals are selected, proving that unselected inputs are
  ignored;
- selecting a zero-volt external CV for either oscillator produces the same
  state as explicitly disabling that oscillator's cross-modulation, while
  differing from the non-zero triangle normal; an additive implementation
  cannot pass this check;
- an external clock edge is ignored while the oscillator-2 clock normal is
  selected, and the next known oscillator-2 rising edge clocks the register;
- holding the selected external clock low suppresses that same internal edge;
  and
- an external Schmitt rising edge clocks the register while the external route
  is selected and the internal oscillator has not produced an edge; and
- a second identically configured voice receiving the normalized V1
  PWM/RUNCV forcing through its external audio input matches LP, BP, and HP
  samples within ten microvolts after final output calibration (one microvolt
  at the unnormalized filter core) for 8,192 Eco-mode frames; and
- a nominal +/-5 V external square at 1x drive and 62% resonance spends no
  more than 1% of a long render in the output limiter.

The clock checks start from register state `0x01` at 1,024 Hz with oscillator 2
at 256 Hz. The expected single direct-recirculation shift is therefore the
unambiguous state `0x80`.

Run the retained verification with:

```sh
make test
```

## Acceptance conclusion

The direct implementation and focused host regression test retain AC-010's
replacement semantics and separately prove the corrected internal filter
source against an external reference signal.
