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
crossfade fully internal, it receives the bipolar PWM comparator plus `0.10`
times the stepped-CV value from the start of the current internal step. With
the crossfade fully external, it receives the conditioned external signal.

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
- a second identically configured voice receiving `PWM + 0.10 * previous
  stepped CV` through its external audio input produces bit-exact LP, BP, and
  HP samples for 8,192 Eco-mode frames.

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
