# Release gate

This record captures the distribution decisions approved for Burl. It is a
project risk record, not legal advice.

## Recorded decisions

- **Public name:** Burl.
- **Release version:** 1.0.1.
- **Release author:** Neal Sanche.
- **Project homepage:** <https://github.com/thorinside/burl>.
- **Repository tags:** `disting-nt`, `eurorack`, `synthesizer`, `audio-plugin`,
  `dsp`, `chaotic-synthesizer`, `feedback-shift-register`, `cpp`.
- **License:** MIT for project-original software and files expressly covered by
  the license notice.
- **Naming review:** the existing U.S. BURL wordmark is accepted as a known,
  low release risk rather than a blocker, given the free open-source release,
  small disting NT audience, no claim of trademark rights, non-endorsement
  language, and willingness to rename after a credible conflict or objection.
- **Branding:** do not use ™ or ®. Do not imply affiliation, authorization,
  sponsorship, approval, endorsement, or ownership of trademark rights.
- **Terminology:** primary product and UI language is “feedback shift-register
  pattern generator.” “Rungler” is limited to subordinate historical
  attribution.
- **Third-party rights:** do not distribute copied code or protected
  expression. MIT does not license third-party names, schematics, artwork,
  manuals, designs, or trademarks.
- **Compatibility review:** release targets API v13 and firmware 1.17.0. Record
  any API or firmware compatibility changes found during delivery.

## AC-003 review record

- **Review date:** 2026-07-18.
- **Approved Spec:** SHA-256
  `19ddd4d1fc8268c0af9b1cba53d7ff77c2745d1c47fb375f2c3df9029476fb1b`.
- **Release-name review:** Burl is the recorded public name. The existing U.S.
  BURL wordmark was reviewed and accepted by the owner as a known, low risk,
  with no claim of trademark rights and a willingness to rename after a
  credible conflict or objection.
- **Attribution review:** the exact subordinate historical attribution and
  non-endorsement wording approved for release is retained below.
- **Relevant-rights review:** project-original software is MIT-licensed;
  third-party code or protected expression may not be copied; and the MIT
  License does not cover third-party names, schematics, artwork, manuals,
  designs, or trademarks. The official Expert Sleepers `distingNT_API`
  submodule is pinned to commit
  `cd12d876dbe060859828053efab1cbc98c9df251` (`v1.15.0`, API v13). Its MIT
  License and Expert Sleepers Ltd copyright notice are compatible with this
  project's MIT source and binary distribution and are retained in the
  submodule and `THIRD_PARTY_NOTICES.md`.

**Review conclusion:** the public name, attribution wording, and relevant
third-party-rights posture required by AC-003 have been reviewed and recorded.
This conclusion completes the decision gate. Public source or binary
distribution remains a separate owner action and is not performed by this
review.

## Required release attribution

> Burl is an independently developed digital instrument that recreates selected observable behaviours of the Benjolin, a 2009 DIY/workshop instrument designed by Rob Hordijk. The reference architecture combines two wide-range oscillators, a voltage-controlled filter, and Hordijk’s feedback shift-register pattern generator, called the Rungler. This project is not affiliated with, sponsored by, approved by, or endorsed by the original designer’s successors, After Later Audio, Epoch Modular, or Macumbista. Third-party names are used only to identify the historical instrument studied. The MIT License applies only to this project’s original software and files expressly covered by its license notice.

## Distribution checklist

No public source or binary release may occur until every item below is true:

- [x] Public name and naming-risk posture are recorded.
- [x] Required attribution and non-endorsement wording are recorded.
- [x] Scope of the MIT License and relevant third-party rights are recorded.
- [x] Every distributed dependency and file has a compatible, recorded license.
- [x] The release source, plug-in UI, and binary use Burl as the primary brand
      without trademark symbols; the release README carries the required
      attribution for distribution with them.
- [x] The release build record identifies the compiler/toolchain version, API
      version, required flags, and source commit.
- [x] Host and hardware verification required by the approved Spec is complete
      and retained.

## Version 1.0.0 release record

- **Public release tag:** `v1.0.0`, resolving to source commit
  `cfafd2caba73071876b920ee92f7144965c7f3c9`.
- **Hardware-validation baseline:**
  `7def2805849cd98dc77b7ee6d860e054db54b1b5`, the last implementation and
  verified-object change before the release documentation and automation.
- **Hardware-validated object:** `plugins/Burl.o`, SHA-256
  `92cbfbb2fff6f9244060057dc5e36ecd73b04717ab714810cd64fac04967c24b`.
- **Hardware-validation toolchain:** `arm-none-eabi-g++` 15.2.1 20251203.
- **Public GitHub build:** Actions run
  `29668621762` on Ubuntu 24.04 with `arm-none-eabi-g++` 13.2.1 20231009.
  The hosted `Burl.o` SHA-256 is
  `e99e94514f463d3d5e51b843187fad216485a440101caf9d57886794a519d5a0`;
  the hosted `Burl-plugin.zip` SHA-256 is
  `457fc9108a182f3e834ae930341468f000ca51f0e30706cbc6a8dceba5e9236a`.
- **Target and flags:** C++11, Cortex-M7, Thumb, FPv5-D16 hard float,
  position-independent code, function/data sections, no RTTI, exceptions, or
  unwind tables, and a relocatable no-standard-library link. The authoritative
  flags are `HARDWARE_FLAGS` and `HARDWARE_LDFLAGS` in `Makefile`.
- **Host contract:** Expert Sleepers API v13 on disting NT firmware 1.17.0,
  using the pinned API dependency recorded above.
- **Verification:** the complete host, sanitizer, native-build, branding,
  preset, hardware, CPU, and owner-listening evidence required by the approved
  Spec has been retained.

## AC-025 branding verification

The release-branding audit was completed on 2026-07-18 against approved Spec
SHA-256 `19ddd4d1fc8268c0af9b1cba53d7ff77c2745d1c47fb375f2c3df9029476fb1b`.
The audit establishes that:

- the repository title, factory name, on-device title, implementation naming,
  and installable `plugins/Burl.o` identify the product as Burl;
- neither trademark symbol appears on the release-facing product surfaces or
  in the installable object;
- Benji, Benjolin, and Rungler do not appear in the implementation, plug-in UI,
  factory metadata, or installable object; product-facing uses are limited to
  subordinate historical attribution, while provenance and audit fixtures
  retain the terms only as evidence;
- the README and this release record contain the exact approved subordinate
  historical attribution and explicit non-endorsement wording; and
- this record retains the owner's acceptance of the U.S. BURL wordmark as a
  known, low release risk, makes no claim of trademark rights, and records the
  willingness to rename after a credible conflict or objection.

`make branding-check` verifies these source, documentation, metadata, and Arm
object properties. It is also part of `make verify`, so a missing attribution,
legacy primary product name, trademark symbol, renamed binary, or branding
regression fails release verification.

This evidence completes AC-025's branding requirement. Together with the
dependency, build, host, hardware, and owner records above, the distribution
gate is satisfied.

Version 1.0.0 is release-ready. Public releases are created only from explicit
`v*` version tags: `.github/workflows/release.yaml` repeats full verification,
packages `programs/plug-ins/Burl.o` as `Burl-plugin.zip`, and attaches the
archive to the matching GitHub Release.

## Version 1.0.1 filter-character release record

Version 1.0.1 preserves the public `ThBu` factory GUID, all 50 positional
parameters and defaults, pages, routes, and v1 preset format. It changes
internal filter sound and response and permits `None` on every output without
changing any default route. It adds no self-oscillation, character, or DC-block
parameter; LP, BP, and HP stay DC-coupled, and users can AC-couple downstream
when required.

- **Implementation and hardware baseline:**
  `ca97c425acc9390d0ce2715c3b13ed6967061f19`.
- **Hardware-validated local object:** 8,761 bytes by section total, SHA-256
  `107869f478d5c23583945ad30898dd679bc053293f6c69de338a7c34f25e4dce`.
- **Verification:** `make verify` passed the host, sanitizer, deterministic WAV,
  five-rate 16x oracle, native build, undefined-symbol, size, branding, preset,
  routing, reset, and safety gates.
- **Physical listening:** NT Push reported `Success!`; the connected module
  exposed the expected `ThBu` / Burl slot, and the owner reported, "Sounds
  great," after hearing the final output stage.
- **Processor load:** one instance measured 25% total in Eco, 45% total in
  Normal, and 84% total in High. Quality was restored to Normal after the
  reversible check.
- **Audio evidence:** matched v1.0.0, faulty-candidate, corrected-candidate, and
  independent-reference WAVs and metrics are retained in
  `verification/theoretical-audio`. No physical WAV was used to tune the
  release; direct owner listening supplied the physical acceptance judgment.
- **Public release tag:** `v1.0.1`, resolving to source commit
  `d351fe935e29a7c0c1d12a1ca43cc2e2445797de`.
- **Public GitHub build:** Actions run `29703223088` on Ubuntu 24.04 with
  `arm-none-eabi-g++` 13.2.1 20231009 completed successfully. The downloaded
  hosted `Burl.o` is 8,759 bytes by section total with SHA-256
  `b4cf7203c0c2c1bce551b335445a15dc0a83539e863bf2e273d73137d1f571e1`;
  the hosted `Burl-plugin.zip` SHA-256 is
  `df165ef802fa81f1df2c9f5c58e5be26a18c6eb06819c05b42705926e2be8b1d`.
- **Public release:** [Burl 1.0.1](https://github.com/thorinside/burl/releases/tag/v1.0.1)
  is neither a draft nor a prerelease and is the repository's latest release.

The owner authorized shipment on 2026-07-19. The first tag run stopped before
packaging because the workflow lacked NumPy for the new audio verifier; no
release or asset was created. Commit
`d351fe935e29a7c0c1d12a1ca43cc2e2445797de` added that release dependency, and
the replacement tag run completed the verified public release recorded above.
