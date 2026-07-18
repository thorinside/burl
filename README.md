# Burl

Burl is a self-contained chaotic synthesizer voice for the Expert Sleepers
disting NT native C++ plug-in environment. It combines two cross-modulating
triangle oscillators, an eight-bit feedback shift-register pattern generator,
a triangle comparator, and a resonant multimode filter.

## Project status

Burl is in active development. The repository currently contains the first
host-tested DSP slice: the deterministic feedback shift-register pattern
generator. **It is not yet a loadable disting NT plug-in and is not ready for
hardware use or public binary distribution.**

The release target is disting NT C++ API v13 on firmware 1.17.0. The eventual
hardware product will be a C++11 Cortex-M7 hard-float position-independent
relocatable `Burl.o` plug-in. Released parameter order and the factory GUID will
be frozen when that integration is introduced.

## Build and test

A C++11 compiler and Make are required for the host tests:

```sh
make test
```

The test suite currently verifies:

- eight-shift direct recirculation at the low Change endpoint;
- complemented recirculation after eight shifts and return after sixteen;
- a 127-state non-zero maximal-length recurrence;
- eight exact monotonic stepped-CV levels spanning -5 V to +5 V;
- deterministic reset and prevention of zero lock in 127 mode;
- repeatability of intermediate Change decisions; and
- bit-exact repeated pattern renders across the supported sample-rate and
  quality-mode contexts.

The retained render matrix and its current product-level limitation are
recorded in [the determinism verification](docs/DETERMINISM.md). No hardware
artifact is produced yet.

## Independence, attribution, and release gate

The implementation is written from project requirements and behavioural
descriptions. No third-party implementation source is copied or mechanically
translated. The project does not currently make a public “clean-room” claim.
See [the provenance record](docs/PROVENANCE.md) and
[the release gate](docs/RELEASE_GATE.md).

Burl is an independently developed digital instrument that recreates selected
observable behaviours of the Benjolin, a 2009 DIY/workshop instrument designed
by Rob Hordijk. The reference architecture combines two wide-range
oscillators, a voltage-controlled filter, and Hordijk’s feedback shift-register
pattern generator, called the Rungler. This project is not affiliated with,
sponsored by, approved by, or endorsed by the original designer’s successors,
After Later Audio, Epoch Modular, or Macumbista. Third-party names are used only
to identify the historical instrument studied. The MIT License applies only to
this project’s original software and files expressly covered by its license
notice.

## License

Project-original source is available under the [MIT License](LICENSE).
Third-party names, schematics, artwork, manuals, designs, and trademarks are
not licensed by that notice.
