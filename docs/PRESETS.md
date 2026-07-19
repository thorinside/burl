# Preset compatibility and Seeded restoration

## Version-one compatibility contract

Burl v1 uses the disting NT host's ordinary preset serialization for all 50
parameter values and their mappings. The plug-in does not add custom JSON or
attempt to restore running mid-cycle DSP state. Its preset identity is the
frozen factory GUID `ThBu`, and the declaration order and names of all 50
parameters in `src/plugin.cpp` are the frozen positional v1 parameter ABI.

The source now guards that contract in two places:

- compile-time assertions pin the parameter count and boundary indices; and
- `tests/plugin_integration_test.cpp` pins the GUID and every parameter name at
  every positional index.

Released parameters must not be reordered or renamed. Any future compatible
parameter additions must preserve all existing indices and require a separate
compatibility review.

All nine input routes and all eight output routes accept the host value `0`
(`None`). Output defaults remain Outputs 1-8, so existing preset values and the
positional ABI are unchanged; the wider minimum only makes disconnected output
routing representable.

## Seeded restoration behavior

`Seed` is an ordinary host-managed parameter. When the host reports that it
changed, `parameterChanged()` first applies the complete host value array, then
sets the configured seed and calls `Voice::reset()`. Reset restores both
oscillators, the feedback shift register, the stepped CV, the filter
integrators, clock/reset edge detectors, and the bounded display status. The
restored `Steps mode` is already applied, so a zero-equivalent seed cannot lock
127 mode.

This is intentionally **Seeded** restoration: loading the same preset starts a
known deterministic run from its configured seed. It does not resume the
precise oscillator, filter, or register phase present when the preset was
saved.

## Retained automated evidence

The plug-in integration test uses a non-default snapshot covering all 50
positions, including Seed 113, 127 mode, High quality, all nine external input
routes, all eight output routes, and Replace output modes. It:

1. renders from the configured seed;
2. advances and changes every determinism-relevant subsystem;
3. restores the complete positional parameter array into the same instance;
4. invokes parameter callbacks in reverse order to avoid relying on declaration
   order; and
5. compares the complete 64-bus render bit for bit with both the original
   seeded render and a freshly constructed instance located through `ThBu`.

The test also asserts that Burl has no custom `serialise` or `deserialise`
callback. Ordinary parameter values and mapping records therefore remain in
one authoritative format owned by the disting NT host rather than being
shadowed by plug-in JSON.

Run the retained check with:

```sh
make test
```

## Completed live-host evidence

The target-hardware round-trip was completed on 2026-07-18 with firmware
1.17.0. The temporary `/presets/Burl AC023 Validatio.json` fixture was saved
with factory GUID `ThBu`, `Seed` 113, `Change` 321, High quality, Osc 1 CV
input routed to Input 1, and Change assigned to performance index 1. Exporting
the saved file through nt_helper confirmed those values in the 50-position
parameter array, the `perfPage` record for slot 0 parameter 3, and the active
engine settings of 48 kHz with a 32-sample block.

The separate Init preset provided a changed running state: `Seed` 93,
`Change` 0, Normal quality, Osc 1 CV input None, and no performance mapping.
Two Init-to-validation loads were performed. Both restored `Seed` 113,
`Change` 321, High quality, Input 1 routing, and Change on performance index 1.
The retained bit-exact integration test supplies the corresponding seeded
render comparison, while the product owner's live listening confirmed that
the loaded plug-in produced active, varied outputs on the module.

After validation the module was restored to Init with one Normal Burl instance
and no performance mapping. The validation preset remains on the SD card as a
small repeatable regression fixture.
