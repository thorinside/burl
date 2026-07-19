# Native build and hardware verification

## Reproducible build inputs

The first native integration was verified on 2026-07-18 with:

- official `expertsleepersltd/distingNT_API` Git submodule commit
  `cd12d876dbe060859828053efab1cbc98c9df251` (`v1.15.0`), whose current API is
  v13;
- `arm-none-eabi-g++` 15.2.1 20251203;
- C++11, Cortex-M7, Thumb, FPv5-D16 hard-float, position-independent code;
- function/data sections, no RTTI, exceptions, or unwind tables; and
- a relocatable, no-standard-library link.

The exact flags are the `HARDWARE_FLAGS` and `HARDWARE_LDFLAGS` definitions in
the repository `Makefile`. Reproduce the retained checks with:

```sh
git submodule update --init --recursive
make verify
```

The verified `plugins/Burl.o` is 7,711 bytes by section total and has SHA-256
`92cbfbb2fff6f9244060057dc5e36ecd73b04717ab714810cd64fac04967c24b`.
The unresolved symbols reported by `make check` are limited to disting NT host
services, position-independent linkage, memory intrinsics, and firmware math
runtime functions.

The hardware-accepted 1.0.1 implementation baseline
`ca97c425acc9390d0ce2715c3b13ed6967061f19` is 8,761 bytes by section total and
has local SHA-256
`107869f478d5c23583945ad30898dd679bc053293f6c69de338a7c34f25e4dce`.

## API and host integration checks

The retained plug-in integration test verifies:

- `pluginEntry()` reports API v13 and one `ThBu` / `Burl` factory;
- the instrument and filter/EQ tags, 50 parameters, and 11 pages;
- all nine optional inputs default to `None` and cannot index before bus 1;
- all eight outputs allow `None`, default to hardware Outputs 1-8, can address
  all 64 buses, and implement independent Add and Replace mixing;
- processing uses `numFramesBy4 * 4` rather than a fixed frame count;
- construction uses `NT_globals.sampleRate`; and
- draw and custom parameter strings execute under the host harness.

The full `make verify` run passed the deterministic, reset, PWM, routing,
stress, ASan, UBSan, API integration, Arm build, undefined-symbol, and size
checks before the hardware object was pushed.

## Live disting NT results

NT Push transferred `plugins/Burl.o` to `/programs/plug-ins/Burl.o`, requested a
plug-in rescan, and reported `Success!`. The product owner then loaded Burl on
the connected module and reported that it loaded and produced many unusual
outputs. A direct version-string request and nt_helper inspection recorded:

- firmware `v1.17.0`, built 2026-07-01 16:29:04;
- Init preset, slot 1, factory `ThBu`, name `Burl`;
- 51 host-visible parameters including Bypass (50 plug-in parameters);
- all nine optional inputs at `None`;
- LP, BP, HP, stepped CV, PWM, XOR, Aux A, and Aux B routed to hardware Outputs
  1 through 8 respectively; and
- one Normal instance at 36% slot CPU (36.5% total in the observed sample).

Reversible performance checks were made without saving the Init preset:

- one High instance: 65-66% slot CPU, 67% total;
- two Eco instances: 21-22% per slot, 44% total; and
- two Normal instances: 75% total, observed independently by the product
  owner.

The temporary second instance was removed and slot 1 was restored to Normal.
The final inspected state was one Burl instance at 36% slot CPU. A preset
export from the same live session recorded the active engine at 48 kHz with a
32-sample block.

These results establish that the API-v13 object scans, loads, produces live
outputs, meets the single-High CPU ceiling, and restores host-managed values,
routing, and a performance mapping across repeated preset loads. The product
owner subsequently completed the separate listening, two-Eco dropout, display,
external-clock, and reset checks in the approved Portal acceptance forms. The
retained positional-ABI, Seeded-restoration harness, and live preset round-trip
are documented in [PRESETS.md](PRESETS.md).

## Version 1.0.1 hardware acceptance

On 2026-07-19, NT Push loaded the exact local object above and reported
`Success!`. A live nt_helper inspection then found one connected `ThBu` / Burl
slot with all nine optional inputs at `None`, the eight outputs still routed to
Outputs 1-8, Quality restored to Normal, and the owner's sound patch unchanged
apart from the intended plug-in update.

The owner listened to the final Eurorack-level filter output on the physical
module and reported, "Sounds great," authorizing shipment. Reversible live
Quality checks measured the updated one-instance CPU envelope as:

- Eco: 25% slot, 25% total;
- Normal: 44-45% slot, 45% total; and
- High: 82% slot, 84% total.

High remained below the module's critical range. The check restored the exact
Normal value before release preparation. These measurements replace the 1.0.0
numbers for the filter-character implementation; the earlier two-Eco dropout
acceptance remains the retained multi-instance baseline.
