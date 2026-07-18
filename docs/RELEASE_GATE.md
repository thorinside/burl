# Release gate

This record captures the distribution decisions approved for Burl. It is a
project risk record, not legal advice.

## Recorded decisions

- **Public name:** Burl.
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
  designs, or trademarks. The Expert Sleepers API header is not currently
  vendored and its version and license must be recorded if it becomes a
  distributed dependency.

**Review conclusion:** the public name, attribution wording, and relevant
third-party-rights posture required by AC-003 have been reviewed and recorded.
This conclusion completes that decision gate but does not authorize a release:
the remaining unchecked delivery evidence below must still be completed. No
public source or binary distribution is performed by this review.

## Required release attribution

> Burl is an independently developed digital instrument that recreates selected observable behaviours of the Benjolin, a 2009 DIY/workshop instrument designed by Rob Hordijk. The reference architecture combines two wide-range oscillators, a voltage-controlled filter, and Hordijk’s feedback shift-register pattern generator, called the Rungler. This project is not affiliated with, sponsored by, approved by, or endorsed by the original designer’s successors, After Later Audio, Epoch Modular, or Macumbista. Third-party names are used only to identify the historical instrument studied. The MIT License applies only to this project’s original software and files expressly covered by its license notice.

## Distribution checklist

No public source or binary release may occur until every item below is true:

- [x] Public name and naming-risk posture are recorded.
- [x] Required attribution and non-endorsement wording are recorded.
- [x] Scope of the MIT License and relevant third-party rights are recorded.
- [ ] Every distributed dependency and file has a compatible, recorded license.
- [x] The release source, plug-in UI, and binary use Burl as the primary brand
      without trademark symbols; the release README carries the required
      attribution for distribution with them.
- [ ] The release build record identifies the compiler/toolchain version, API
      version, required flags, and source commit.
- [ ] Host and hardware verification required by the approved Spec is complete
      and retained.

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

This evidence completes AC-025's branding requirement. It does not authorize a
public release: the unchecked dependency, build-record, and remaining host and
hardware gates above are still mandatory.

The checked discovery decisions satisfy the decision portion of the gate. The
unchecked delivery evidence remains mandatory; this repository is not yet
release-ready.
