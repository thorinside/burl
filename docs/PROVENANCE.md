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
copied, or mechanically translated while producing it. Commit history is the
authorship record for project files.

The Expert Sleepers API header will be used as a build interface when the
native plug-in adapter is added; that dependency is not currently vendored.
Its version and license must be recorded before distribution.

This record supports an **independently developed** description. It is not, by
itself, sufficient evidence for the stronger public phrase “clean-room,” and
the project does not currently use that phrase as a release claim.
