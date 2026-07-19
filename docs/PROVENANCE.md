# Implementation provenance

## Requirements basis

- Project: Burl (internal historical codename: Benji)
- Administrator-approved Spec SHA-256:
  `19ddd4d1fc8268c0af9b1cba53d7ff77c2745d1c47fb375f2c3df9029476fb1b`
- Implementation record opened: 2026-07-18
- Behavioural basis: the approved Spec's signal relationships, recurrences,
  endpoint behaviours, deterministic reset rules, and acceptance criteria

## Source-access and authorship record

The project-original implementation in this repository was authored from the
requirements basis above. No source from a third-party emulation was pasted,
copied, mechanically translated, or otherwise used while producing it. Commit
history is the authorship record for project files.

The native plug-in adapter uses the official Expert Sleepers `distingNT_API`
Git submodule as its build interface. It is pinned to
`cd12d876dbe060859828053efab1cbc98c9df251` (`v1.15.0`, API v13). The dependency
is MIT-licensed, with copyright held by Expert Sleepers Ltd; its terms are
compatible with this project's MIT distribution and its notice is retained in
the submodule and `THIRD_PARTY_NOTICES.md`.

This record supports an **independently developed** description. It is not, by
itself, sufficient evidence for the stronger public phrase “clean-room,” and
the project does not currently use that phrase as a release claim.

## Independent-development review

Review date: 2026-07-18

Implementation baseline reviewed:
`8e5c87d998fceb7dafb0ee278c61f14a015b2cc9`

The review covered the complete implementation present at that baseline:

- `include/burl/pattern_generator.hpp`
- `src/pattern_generator.cpp`
- `tests/pattern_generator_test.cpp`
- the build, license, README, and release/provenance records that define how
  those files are compiled and described

Review evidence and findings:

1. The repository history begins with the reviewed baseline. It records all
   reviewed files as newly authored project files in one non-merge commit by
   the recorded repository author; there is no imported parent history.
2. At that baseline the checkout contained no submodules, vendored source,
   generated source, tracked symbolic links, or project remote. Implementation
   includes were limited to the project header and C++ standard-library
   headers.
3. A source-reference and notice scan found no third-party source URL,
   third-party copyright notice, or copied-code attribution in the
   implementation. Historical names occur only in the approved subordinate
   attribution and project policy records.
4. The implementation expresses the approved behavioural requirements
   directly: eight/sixteen recirculation, the isolated 127-state recurrence,
   the three-tap stepped-CV mapping, and deterministic reset. Host tests derive
   their expectations from the same approved acceptance criteria.
5. The retained authoring declaration above records that no third-party
   emulation source was copied, mechanically translated, or otherwise used to
   create the implementation.

**Review conclusion:** the complete implementation at the recorded baseline is
independently written from the approved behavioural Spec, with no evidence of
copied or mechanically translated third-party emulation source. This completes
AC-002 for that baseline. Future implementation commits must preserve this
separation and extend this record before AC-002 is reassessed for a release.

## Integrated-voice extension review

Review date: 2026-07-18

Implementation baseline reviewed:
`ecf3e1830a8334ddf5207fe90c57f30ca9f031be`

This delta review extends the earlier review rather than repeating it. It covers
all implementation and test changes after `8e5c87d998fceb7dafb0ee278c61f14a015b2cc9`
through the new baseline, including:

- `include/burl/voice.hpp`
- `src/voice.cpp`
- `tests/determinism_test.cpp`
- `tests/voice_determinism_test.cpp`
- associated build and determinism documentation changes

Review evidence and findings:

1. The integrated voice and determinism tests were authored directly from the
   approved Spec and the pre-existing project-owned pattern-generator API. No
   third-party emulation source was accessed, copied, or mechanically
   translated for this extension.
2. At that baseline production includes remained limited to project headers and
   the C++ standard library. Test includes were likewise limited to project
   headers and standard library facilities; no vendored, generated, or
   externally sourced code was introduced.
3. Repository boundary checks at that baseline found no remote, submodule,
   tracked symbolic link, or external source notice. Every new C++ source and
   header carried the project MIT SPDX and copyright notice.
4. The delta expresses approved behavioural relationships directly: reflected
   triangle oscillators, internal-normal replacement, deterministic register
   clocking, triangle comparison, state-variable filtering, common quality
   rates, low-pass decimation, and bounded scalar state.
5. Commit history retains the complete authorship delta, and the zero-tolerance
   independent-render tests retain executable evidence of the implementation's
   deterministic behavior.

**Review conclusion:** the independently developed finding now covers the
complete implementation through the recorded integrated-voice baseline. No
evidence of copied or mechanically translated third-party emulation source was
found. This extends AC-002 evidence to that baseline; later implementation
changes must receive another delta review before release reassessment.

## Native adapter and version 1 release review

Review date: 2026-07-18

Implementation and verified-object baseline reviewed:
`7def2805849cd98dc77b7ee6d860e054db54b1b5`

This release delta review covers the native adapter, the official API
submodule, host integration and sanitizer tests, live-hardware evidence, preset
compatibility, quality switching, and release branding added after the
integrated-voice review.

1. Project-owned production and test code remains independently written from
   the approved behavioural Spec. No third-party Benjolin-emulation source was
   added or referenced.
2. The only third-party source dependency is the official Expert Sleepers
   `distingNT_API` submodule pinned to
   `cd12d876dbe060859828053efab1cbc98c9df251`. It supplies the documented host
   ABI and examples; it is not a Benjolin implementation.
3. The dependency's MIT License is compatible with distribution of Burl under
   MIT. Its Expert Sleepers Ltd copyright and license terms are retained in the
   submodule and `THIRD_PARTY_NOTICES.md`.
4. The relocatable plug-in object has no statically linked C++ standard
   library. Its unresolved external symbols are the documented disting NT host,
   PIC, memory-intrinsic, and firmware math-runtime services.
5. All project-owned C++ sources and headers retain the project MIT SPDX and
   copyright notice. The release attribution continues to separate the scope
   of that license from third-party names and protected expression.

**Review conclusion:** the independent-development finding and dependency
rights record cover the complete Burl 1.0.0 implementation and verified object
at the recorded baseline. The pinned API dependency is compatible with the
planned MIT release, and no unrecorded distributed source dependency remains.

## Filter-character candidate delta review

Review date: 2026-07-19

Candidate branch reviewed: `codex/filter-character`

This delta review covers the extracted filter, corrected internal source,
resonance and drive behavior, and their focused tests and documentation:

- `include/burl/filter.hpp`
- `src/filter.cpp`
- `include/burl/voice.hpp` and `src/voice.cpp`
- `tests/filter_test.cpp`
- filter source-routing and native parameter integration fixtures

The initial implementation was authored from the approved behavioral plan and
the repository's existing project-owned DSP interfaces. After the static bug
was reproduced, the correction was independently derived from Rob Hordijk's
published 2009 schematic and technical notes, licensed-product manuals, and the
SSM2164 manufacturer data sheet. The consulted primary sources and the boundary
between explicit facts and circuit inference are recorded in
`docs/FILTER_REFERENCE_RESEARCH.md`. No third-party emulation source was
accessed, copied, or mechanically translated. The only compiled external source
dependency remains the pinned official Expert Sleepers API submodule.

The filter implementation, high-rate renderer, analysis scripts, lookup
values, protection curves, and tests are project-original code. The V1 input
coefficients and fixed character-path magnitude are independently calculated
from published component values; the Q curve, quadratic input compensation,
Input drive, limiter curves, and fixed 10x Eurorack output calibration remain
project-specific choices rather than claims about the original circuit. All
new C++ sources retain the project MIT SPDX and copyright notice, and the
frozen factory and positional parameter ABI remain unchanged.

**Review conclusion:** the independent-development finding extends to Burl
1.0.1. The fixed implementation baseline is
`ca97c425acc9390d0ce2715c3b13ed6967061f19`; full host/native verification,
physical-module listening, updated processor-load measurements, and the local
artifact hash are retained in the release records.
