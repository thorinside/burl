# Runtime quality switching verification

## Runtime contract

Burl exposes Eco, Normal, and High as one host-managed `Quality` parameter.
They select one, two, or four common internal DSP substeps per host frame.
`Voice` owns the oscillator, filter, feedback shift-register, edge-detector, and
display state inline; it has no quality-specific buffer or dynamic storage.

Changing `Quality` invokes the existing `parameterChanged()` callback. That
callback maps the host integer to `QualityMode` and copies the resulting
`VoiceParameters` into the already-constructed `Voice`. It does not call the
factory constructor, `Voice` constructor, or `reset()`. The following
`step()` calls use the new scalar substep count and preserve the running DSP
state.

The factory declares all per-instance storage before construction:

- SRAM: one `BurlAlgorithm`;
- DTC: one `burl::Voice`;
- DRAM: zero bytes; and
- ITC: zero bytes.

High therefore uses the same declared storage as Eco and Normal. Its four
substep results are accumulated in fixed local scalar structures and boxcar
decimated to one host-rate frame.

## Retained automated evidence

`tests/plugin_integration_test.cpp` constructs an instance inside exactly the
SRAM and DTC payload sizes returned by `calculateRequirements()`, with 64-byte
canaries immediately before and after both payloads. After a 2,048-frame
Normal-quality pre-roll, it changes quality for 192 consecutive 64-frame
blocks. The sequence gives each quality 64 blocks and covers 12,288 switched
frames.

For every change and processed block, the test proves that:

- the algorithm address, parameter table, parameter pages, and host value
  pointers are unchanged;
- the active display status and eight-bit pattern are unchanged by the
  parameter callback itself, demonstrating that running state was not reset;
- neither the quality callback nor `step()` invokes the test process's C++ heap
  allocation operators;
- every output frame remains serviced, finite, safety-bounded, and non-silent
  across the eight Replace-routed outputs; and
- the SRAM and DTC canaries remain intact and a repeated requirements query
  returns the same declaration.

The plug-in integration executable runs under AddressSanitizer and
UndefinedBehaviorSanitizer. In addition, `make check-allocations` inspects the
final Cortex-M7 relocatable object and fails if it has an unresolved C or C++
dynamic-allocation symbol. This covers direct C allocation calls that the host
C++ allocation counter would not observe.

Run all retained evidence with:

```sh
make verify
```

## Acceptance conclusion

The implementation path, guarded runtime transition matrix, sanitizer run, and
final-object symbol audit complete AC-024: changing Quality during operation
does not reconstruct the algorithm, interrupt processing, allocate memory in
`step()`, or exceed predeclared memory requirements.
