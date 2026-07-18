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

The Expert Sleepers API header will be used as a build interface when the
native plug-in adapter is added; that dependency is not currently vendored.
Its version and license must be recorded before distribution.

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
2. The checkout contains no submodules, vendored source, generated source,
   tracked symbolic links, or project remote. Implementation includes are
   limited to the project header and C++ standard-library headers.
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
