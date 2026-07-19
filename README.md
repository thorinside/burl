# Burl

Burl is a self-contained chaotic synthesizer voice for the Expert Sleepers
disting NT native C++ plug-in environment. It combines two cross-modulating
triangle oscillators, an eight-bit feedback shift-register pattern generator,
a triangle comparator, and a resonant multimode filter.

## Project status

Burl 1.0.0 is a release-ready disting NT C++ API v13 plug-in with factory GUID
`ThBu` and a host-tested integrated DSP voice. The plug-in exposes 50
parameters on 11 standard pages, nine optional input routes, and eight
independently routed Add/Replace outputs.

The relocatable Cortex-M7 object has been built, pushed, scanned, and loaded on
disting NT firmware 1.17.0. Live hardware produced all eight default outputs;
the retained build and bench record is in
[the native build and hardware verification](docs/NATIVE_BUILD_AND_HARDWARE.md).
The product owner completed the listening, control, preset, and processor-load
acceptance checks on the target module. The project is prepared for a free,
open-source MIT release. Version-tagged builds and release downloads are hosted
on [GitHub Releases](https://github.com/thorinside/burl/releases).

The `codex/filter-character` branch is an unreleased filter-character
candidate. It corrects the internal PWM/stepped-CV source, resonance ping
response, and level-matched transparent `Input drive` behavior while retaining the frozen
`ThBu` identity and all 50 positional parameters. LP, BP, and HP remain
DC-coupled; LP can carry slow or steady offsets by design. Every input and
output route supports `None`, while the eight output defaults remain hardware
Outputs 1-8. This candidate must
pass owner listening and processor-load acceptance on the physical module
before a follow-up release is cut. See the
[filter verification record](docs/FILTER_CHARACTER.md) and
[unreleased notes](CHANGELOG.md).

## Release metadata

- **Version:** 1.0.0
- **Author:** Neal Sanche
- **Homepage:** <https://github.com/thorinside/burl>
- **License:** MIT
- **Repository tags:** `disting-nt`, `eurorack`, `synthesizer`, `audio-plugin`,
  `dsp`, `chaotic-synthesizer`, `feedback-shift-register`, `cpp`
- **Version 1.0.0 tag:** `cfafd2caba73071876b920ee92f7144965c7f3c9`
- **Hosted object SHA-256:**
  `e99e94514f463d3d5e51b843187fad216485a440101caf9d57886794a519d5a0`
- **Hosted archive SHA-256:**
  `457fc9108a182f3e834ae930341468f000ca51f0e30706cbc6a8dceba5e9236a`
- **Hardware-validation baseline:**
  `7def2805849cd98dc77b7ee6d860e054db54b1b5`

## Install

Download `Burl-plugin.zip` from the desired
[GitHub Release](https://github.com/thorinside/burl/releases), extract it at the
root of the disting NT SD card, and rescan plug-ins. The archive installs
`programs/plug-ins/Burl.o`.

## Build and test

A C++11 host compiler, GNU Arm Embedded C++ compiler, and Make are required for
full verification. Clone with submodules, or initialize the pinned official
disting NT API checkout before building:

```sh
git submodule update --init --recursive
make verify
```

`make verify` runs the complete host/sanitizer suite, deterministic theoretical
audio checks, and the five-rate 16x filter oracle; cross-compiles
`plugins/Burl.o`; checks its unresolved firmware/runtime symbols; and reports
its section sizes. With a connected module, `make push` transfers the object
using NT Push. Generated objects remain ignored by Git.

Pushing a version tag such as `v1.0.0` runs the same verification on GitHub
Actions, packages the object in the disting NT SD-card layout, and creates a
GitHub Release with the zip attached.

The test suite currently verifies:

- eight-shift direct recirculation at the low Change endpoint;
- complemented recirculation after eight shifts and return after sixteen;
- a 127-state non-zero maximal-length recurrence;
- eight exact monotonic stepped-CV levels spanning -5 V to +5 V;
- complete deterministic voice reset and prevention of zero lock in 127 mode;
- repeatability of intermediate Change decisions;
- bit-exact repeated pattern renders across the supported sample-rate and
  quality-mode contexts;
- bit-exact repeated integrated-voice renders of all eight outputs at 32,
  44.1, 48, 88.2, and 96 kHz in Eco, Normal, and High modes;
- exact bipolar PWM triangle-comparator behavior, including equality and a
  triangle crossing with unchanged oscillator pulse directions;
- the curved Q 0.5..20 resonance mapping, 250 Hz ping tails, silence stability,
  DC-coupled LP/BP/HP behavior, high-resonance harmonic character, and bounded
  oversampled output limiting;
- the V1 resistor-network PWM/previous-stepped-CV forcing against a
  level-matched external reference render;
- the actual NT `Input drive` parameter mapping from 0.25x through 4x,
  transparent nominal-level gain, and out-of-range input protection;
- replacement (not addition) of both oscillator triangle normals and the
  oscillator-2 clock normal when their external routes are selected; and
- API v13 factory metadata, parameter/page layout, all nine `None`-safe input
  routes, all eight `None`-safe output routes, 64-bus output addressing, and
  Add/Replace output mixing;
- the frozen `ThBu` factory/positional parameter ABI and bit-exact Seeded
  restoration after a simulated host parameter round-trip;
- allocation-free, state-preserving runtime Quality changes across the exact
  declared SRAM and DTC footprints; and
- safety-limited stress operation at all five supported sample rates and all
  three quality modes, including AddressSanitizer and UndefinedBehaviorSanitizer
  verification; and
- release branding across the README, implementation, on-device UI, factory,
  and installable object, plus the exact approved attribution and naming-risk
  record.

The retained render matrices and zero-tolerance comparison are recorded in
[the determinism verification](docs/DETERMINISM.md). The exhaustive eight-code
mapping is recorded in the [stepped-CV verification](docs/STEPPED_CV.md), the
comparator crossing in the [PWM verification](docs/PWM_COMPARATOR.md), and
internal-normal replacement in the
[source-routing verification](docs/SOURCE_ROUTING.md), the complete DSP state
restoration in the [reset verification](docs/RESET.md), the positional ABI and
Seeded load behavior in the [preset verification](docs/PRESETS.md), and runtime
quality transitions in the
[quality-switching verification](docs/QUALITY_SWITCHING.md). The full numerical
and sanitizer matrix is recorded in the
[stress verification](docs/STRESS_TEST.md), and the resonance, drive, and DC
contract is recorded in the
[filter-character verification](docs/FILTER_CHARACTER.md). Native build and
live-device results are recorded in
[the hardware verification](docs/NATIVE_BUILD_AND_HARDWARE.md).
The primary-source boundary and circuit derivation are recorded in
[the filter research](docs/FILTER_REFERENCE_RESEARCH.md), and the matched WAVs,
static diagnosis, and high-rate oracle are recorded in
[the theoretical-audio verification](docs/THEORETICAL_AUDIO_VERIFICATION.md).

## disting NT API dependency

The official `expertsleepersltd/distingNT_API` repository is included as the
`distingNT_API` Git submodule and pinned to
`cd12d876dbe060859828053efab1cbc98c9df251` (`v1.15.0`, API v13). The dependency
is MIT-licensed by Expert Sleepers Ltd and is compatible with this project's
MIT distribution. Its retained notice is recorded in
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md). The submodule keeps the
firmware ABI header, examples, and native build surface available to every
checkout without copying or reimplementing the vendor API.

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
