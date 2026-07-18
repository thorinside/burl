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

## API and host integration checks

The retained plug-in integration test verifies:

- `pluginEntry()` reports API v13 and one `ThBu` / `Burl` factory;
- the instrument and filter/EQ tags, 50 parameters, and 11 pages;
- all nine optional inputs default to `None` and cannot index before bus 1;
- all eight outputs default to hardware Outputs 1-8, can address all 64 buses,
  and implement independent Add and Replace mixing;
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
The final inspected state was one Burl instance at 36% slot CPU. The module's
default sample rate is 48 kHz; future performance records must explicitly
confirm the active system sample-rate and buffer-size settings rather than
assuming defaults.

These results establish that the API-v13 object scans, loads, produces live
outputs, and meets the single-High CPU ceiling. Subjective listening details,
dropout observation for the documented two-Eco patch, display interaction,
external-clock/reset behavior, and the live-host preset/mapping round-trip
remain separate owner acceptance checks. The retained positional-ABI and
Seeded-restoration host harness is documented in [PRESETS.md](PRESETS.md).
