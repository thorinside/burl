# Burl on Daisy Patch.Init: I/O and porting feasibility

**Research date:** 2026-07-20
**Decision question:** Does the stock four-knob Patch.Init have enough physical
I/O and control to make Burl musically useful?

## Conclusion

**Yes. A Patch.Init version would be useful, and a one-voice port is technically
credible.** It would not expose every disting NT route simultaneously, but the
stock panel has an unusually good mix for this particular instrument: two
audio outputs for the filter, one CV output for the stepped pattern, and two
gate outputs for pattern-derived logic. Four continuous CV inputs, two gate
inputs, and stereo audio inputs are also enough for a focused external-control
surface.

The best first version is a **sound-first, self-contained Burl**:

The retained `00-Burl4` listening state makes this fit better than the raw
eight-output count suggests: Aux A selected low-pass, Aux B selected band-pass,
and only those two Aux routes were connected to NT outputs 1 and 2. Patch.Init's
stereo codec outputs can therefore reproduce the audible output pair behind
the sound that prompted this research without time-multiplexing or a menu.
This is a **retained owner-session fact**.

| Patch.Init control or jack | Recommended Burl role |
| --- | --- |
| Knobs 1–4 | Oscillator 1 frequency, oscillator 2 frequency, Change, filter cutoff |
| CV inputs 1–4 | Cutoff, Change, resonance, external-input mix modulation |
| Audio inputs L/R | Mono external filter input; sum or select the two physical inputs |
| Gate input 1 | External pattern clock |
| Gate input 2 | Reset |
| Button | Manual reset/reseed |
| Toggle | Internal/external clock selection |
| Audio output L | Low-pass |
| Audio output R | Band-pass |
| CV output | Eight-level stepped CV, remapped from -5…+5 V to 0…5 V; control-rate/last-value unless explicitly scheduled |
| Gate output 1 | Register XOR, block-sampled by default or event-accurate with explicit scheduling |
| Gate output 2 | Block-rate pattern activity pulse; accurate pattern clock/PWM requires explicit scheduling |
| LED | Pattern/clock activity |

That mapping preserves the accepted Burl sound and its most distinctive
external modulation output. The main losses are simultaneous high-pass, raw
audio-rate PWM, and the two selectable auxiliary taps. None of those signals
is lost inside the synthesis engine; they are merely not all available on
jacks at once.

The practical technical risk is **processor headroom, not memory or I/O**.
Burl already runs at 48 kHz on a Cortex-M7 target and the Patch Submodule is a
480 MHz Cortex-M7 platform. Eco should be the first bring-up target, Normal is
plausible, and High should be treated as unproven until measured on the actual
Patch.Init. A tiny profiling firmware, not a paper estimate, should be the
first implementation gate.

## Evidence labels

- **Official hardware fact** means an Electrosmith product page, datasheet,
  schematic, libDaisy source file, or official example.
- **Current-code fact** means the checked-out Burl `main` source and retained
  verification in this repository.
- **Retained owner-session fact** means the recorded `00-Burl4` routing and
  source selection from the user's hands-on NT Helper session.
- **Inference** means a proposed mapping or engineering judgment that still
  needs a firmware prototype or hardware measurement.

## First, which Daisy is in Patch.Init?

Patch.Init is a carrier/interface for the **Daisy Patch Submodule**, not a bare
Daisy Seed. The Patch Submodule is in the same Daisy family and libDaisy
describes it as based on the Seed, but it adds the codec and signal conditioning
needed for direct Eurorack audio, CV, gate, and power connections. The port
should therefore target libDaisy's `DaisyPatchSM`, not `DaisySeed`.

This distinction is an **official hardware fact** from the
[Patch.Init documentation](https://docs.daisy.audio/product/Daisy-Patch-Init/),
the [Patch Submodule documentation](https://docs.daisy.audio/hardware/PatchSM/),
and the official
[`DaisyPatchSM` board support header](https://github.com/electro-smith/libDaisy/blob/c02245d22b38acad3916d9c2f156bcba34fa15af/src/daisy_patch_sm.h).

## Stock Patch.Init resources

The following is the physical stock module, not the larger set of pins that
would be available on a custom Patch Submodule carrier.

| Resource | Stock Patch.Init | Relevant electrical detail |
| --- | --- | --- |
| Processor | 480 MHz Arm Cortex-M7 | Patch Submodule specification |
| Audio | Two inputs and two outputs, 24-bit, up to 96 kHz | Inputs are AC-coupled; outputs are DC-coupled; the Patch Submodule datasheet gives a typical -5…+5 V range |
| Panel pots | Four | Wired through Patch SM `CV_1`…`CV_4`; their 0…5 V travel occupies the nonnegative half of the bipolar input range |
| Continuous CV inputs | Four | Panel jacks use `CV_5`…`CV_8`; bipolar -5…+5 V, 16-bit ADC path, 100 kΩ input impedance |
| Continuous CV outputs | One physical jack | 12-bit, 0…5 V, 100 Ω output impedance |
| Gate inputs | Two | Conditioned Eurorack inputs; nominal 0…5 V |
| Gate outputs | Two | 0…5 V, 100 Ω output impedance |
| Other panel controls | One momentary button, one two-position toggle, one LED | The official hardware-test example assigns the button to B7 and toggle to B8 |
| Storage/memory | MicroSD plus 64 MB SDRAM | Far beyond Burl's scalar-state needs |

Sources: the official [Patch.Init technical specification and assets](https://docs.daisy.audio/product/Daisy-Patch-Init/),
[Patch.Init databrief](https://daisy.nyc3.cdn.digitaloceanspaces.com/products/patch-init/patch_init_databrief-3-13.pdf),
[Patch.Init schematic](https://daisy.nyc3.cdn.digitaloceanspaces.com/products/patch-init/patch_init_schematic.pdf),
and [Patch Submodule datasheet v1.0.5](https://daisy.nyc3.cdn.digitaloceanspaces.com/products/patch-sm/ES_Patch_SM_datasheet_v1.0.5.pdf).

Two details matter for this port:

1. The Patch Submodule has **two** CV DAC channels, but the stock Patch.Init
   exposes only CV output 1 as a jack. Its schematic uses CV output 2 for the
   panel LED. A custom face/carrier could expose both, but the stock module
   should be evaluated as one CV-output module.
2. The two codec audio outputs are DC-coupled. Either one can therefore carry
   Burl's bipolar stepped CV when exact -5…+5 V behavior matters more than a
   second simultaneous filter response.

The current libDaisy implementation initializes Patch SM audio at 48 kHz,
24-bit, with a 48-sample default block; it exposes 48 and 96 kHz (among other
rates), starts the two-channel CV DAC at 48 kHz, and normalizes all eight Patch
SM `CV_n` channels as bipolar values. On Patch.Init the pots occupy `CV_1`…`CV_4`
and the four panel CV jacks occupy `CV_5`…`CV_8`; `ADC_9`…`ADC_12` are not extra
front-panel controls. These are **official software facts** in
[`daisy_patch_sm.cpp`](https://github.com/electro-smith/libDaisy/blob/c02245d22b38acad3916d9c2f156bcba34fa15af/src/daisy_patch_sm.cpp)
and
[`AnalogControl`](https://github.com/electro-smith/libDaisy/blob/c02245d22b38acad3916d9c2f156bcba34fa15af/src/hid/ctrl.cpp).

There is one **local target-hardware fact** to preserve in the adapter. A
one-jack-at-a-time diagnostic on this specific Patch.Init verified that the
physical `CV_5`…`CV_8` labels correspond through libDaisy enum order to Patch
SM pins `C9`, `C8`, `C6`, and `C7`. That differs from the public pin diagram,
but the reconciled local libDaisy scan order already makes the public
`CV_5`, `CV_6`, `CV_7`, and `CV_8` symbols match the physical jacks. The Burl
adapter must read those four libDaisy symbols in order and must not add another
application-side shuffle. The retained evidence is in the sibling target's
`docs/hardware_cv_contract.md` and `AGENTS.md`.

## What the current Burl actually needs

Burl's DSP core is a mono, self-contained voice with two triangle oscillators,
cross-modulation and feedback, an eight-bit pattern generator, a triangle
comparator, and a resonant state-variable filter. The NT adapter exposes 50
host parameters, nine optional logical input routes, and eight signal outputs.
The authoritative definitions are the
[parameter and route table](../src/plugin.cpp),
[voice interface](../include/burl/voice.hpp), and
[integrated signal path](../src/voice.cpp).

### Complete parameter inventory

This is a **current-code fact**. Values in parentheses are current defaults;
frequency ranges show the DSP value after the NT control mapping.

| Group | Parameters |
| --- | --- |
| Main (6) | `Osc 1 frequency` 0.02…8500 Hz (60 Hz); `Osc 2 frequency` 0.02…8500 Hz (47 Hz); `Change` decision range 0…1 (0.5); `Cutoff` 4…16000 Hz (250 Hz); `Resonance` 0…100% (62%); `Input mix` 0…100% (100%, the fully internal Burl source endpoint) |
| Feedback (4) | `Osc 1 feedback` -1…+1 (+0.35); `Osc 2 feedback` -1…+1 (+0.45); `Osc 1 CV amount` -1…+1 (+0.15); `Osc 2 CV amount` -1…+1 (+0.15) |
| Pattern (6) | `Change CV amount` -1…+1 (0); `Steps mode` 8/16 or 127 (8/16); `Clock rate` Single or Double (Single); `DAC taps` 6,7,8 or 2,4,7 (6,7,8); `Seed` 1…255 (93); `Reseed` confirmation (off) |
| Filter (6) | `Stepped to cutoff` -1…+1 (+0.35); `Filter CV amount` -1…+1 (0); `Resonance CV amount` -1…+1 (0); `Mix CV amount` -1…+1 (0); `Input drive` 0.25…4.00x (1x); `Output limit` Off/Soft (Soft) |
| Quality and aux (3) | `Quality` Eco/Normal/High (Normal); `Aux A source` (oscillator 1 triangle); `Aux B source` (oscillator 2 triangle). Either aux can select oscillator 1 triangle/pulse, oscillator 2 triangle/pulse, PWM, XOR, stepped CV, LP, BP, or HP |
| Input routes (9) | `Osc 1 CV input`, `Osc 2 CV input`, `Filter CV input`, `External audio input`, `Clock input`, `Change CV input`, `Resonance CV input`, `Mix CV input`, `Reset input` (all default to None) |
| Output routing (16) | A destination and Add/Replace mode for each of `Low-pass`, `Band-pass`, `High-pass`, `Stepped CV`, `PWM`, `XOR`, `Aux A`, and `Aux B` (default destinations are NT Outputs 1…8, all Add) |

The NT routing and Add/Replace parameters are host integration, not DSP
requirements. A fixed-panel Daisy firmware can replace those 25 route/mode
parameters with a deliberate jack mapping. The remaining settings that do not
fit on the panel can be fixed at good defaults, exposed through a secondary
button layer, or eventually stored on the SD card.

### Nine logical inputs and their musical roles

| Burl logical input | Behavior in the current core | Patch.Init fit |
| --- | --- | --- |
| Oscillator 1 CV | Replaces oscillator 2's internally normalled triangle as oscillator 1's modulation source; it is not an additive calibrated 1 V/oct input | Possible on a CV jack, but omitted from recommended v1 to preserve the internal cross-modulation |
| Oscillator 2 CV | Replaces oscillator 1's internally normalled triangle as oscillator 2's modulation source | Same trade-off as oscillator 1 CV |
| Filter CV | Bipolar cutoff modulation, scaled by `Filter CV amount` | Excellent continuous-CV fit |
| External audio | Crossfades against the internal PWM-plus-stepped-CV filter excitation | Excellent fit for either/both AC-coupled audio inputs |
| Clock | Replaces the oscillator-2 internal clock when enabled; Schmitt thresholds are 2.0 V rising and 0.2 V falling | Excellent gate-input fit, with the toggle selecting internal/external clock because the stock jack provides no documented patch detect |
| Change CV | Modulates the pattern's Change decision | Excellent continuous-CV fit |
| Resonance CV | Modulates filter resonance | Excellent continuous-CV fit |
| Mix CV | Modulates internal/external filter-input balance | Excellent continuous-CV fit |
| Reset | Resets oscillator, filter, register, stepped-CV, and edge-detector state on a Schmitt rising edge | Excellent gate-input and button fit |

The oscillator and clock routes use replacement rather than addition; that is
verified in [the retained source-routing record](SOURCE_ROUTING.md). This is why
the recommended four-CV mapping favors cutoff, Change, resonance, and mix. An
"external oscillator lab" mapping is possible, but it changes the normalled
chaotic interaction whenever those routes are active.

### Eight signal outputs and their musical roles

| Burl output | Signal and use | Best Patch.Init destination |
| --- | --- | --- |
| Low-pass | Main resonant, bass-rich filter voice; deliberately DC-coupled in Burl | Audio output |
| Band-pass | Ping/character output and a strong companion to LP | Audio output |
| High-pass | Brighter/noisier third response from the same filter state | Selectable alternative to LP or BP; no third codec jack |
| Stepped CV | Eight ordered pattern-derived levels spanning exactly -5…+5 V before allowed quality-rate averaging | Physical CV output after a reversible 0…5 V level remap, with explicit DAC scheduling if fast transitions must be retained; or a codec audio output for exact bipolar voltage and timing |
| PWM | Bipolar triangle comparator, nominally -5 or +5 V in Eco; also the important hard-edged internal filter excitation | Optional audio output when raw audio-rate fidelity is wanted; a gate jack is only a logicalized version |
| Register XOR | -5/+5 V comparison of pattern bits 0 and 7; useful as chaotic gate/rhythm | Gate output after conversion to 0/5 V; block-sampled unless its transitions are explicitly scheduled |
| Aux A | Selectable raw oscillator, logic, CV, or filter tap | Omitted from fixed v1 jacks |
| Aux B | Second independently selectable tap | Omitted from fixed v1 jacks |

The output voltage and update facts come from
[`Voice::processInternal()`](../src/voice.cpp),
[stepped-CV verification](STEPPED_CV.md),
[PWM verification](PWM_COMPARATOR.md), and
[filter-character verification](FILTER_CHARACTER.md).

## Mapping options

### Option A — sound-first instrument (recommended)

Use both codec outputs for LP and BP, remap stepped CV to the physical CV jack,
and use the gate outputs for patchable pattern events.

**Why this is the best v1:** LP and BP are the two strongest audible views of
the filter that already sounded good on the NT. The stepped CV retains all
eight codes when transformed as:

```text
patch_init_cv_volts = (burl_stepped_volts + 5) / 2
```

This changes the range from bipolar -5…+5 V to unipolar 0…5 V and halves the
step size. It preserves all eight voltage levels and their ordering
mathematically, but an ordinary held-DAC write once per audio block can skip
intermediate states at fast pattern clocks. XOR maps naturally by sign to a
0/5 V gate, with the same timing qualification.

All three non-audio outputs need an explicit timing policy. GPIO or held-DAC
writes performed rapidly while filling an audio block are not automatically
spaced at the corresponding sample times. A simple adapter should therefore
describe the CV output as last-value/control-rate, Gate 1 as block-sampled XOR,
and Gate 2 as block-rate pattern activity. Event-accurate XOR, the true pattern
clock, fast stepped-CV transitions, or audio-rate PWM require a one-sample
callback or a deliberate timer/DMA scheduling scheme and should be treated as
a follow-up experiment. This timing warning is an **engineering inference**
from the block audio and held-output APIs, not a published bandwidth limit of
the output circuits.

**Simultaneous losses:** HP, raw audio-rate PWM, Aux A, Aux B, and the negative
half of the stepped-CV voltage range. High-pass could replace either LP or BP
through a compile-time choice or a later shifted control layer.

### Option B — modulation-lab breakout

Use one codec output for the selected filter response and spend the other on a
signal that benefits from its 24-bit, DC-coupled, bipolar path:

| Jack | Role |
| --- | --- |
| Audio output L | LP, BP, or HP selected by the toggle/secondary layer |
| Audio output R | Exact bipolar stepped CV **or** raw PWM audio |
| CV output | The remaining logic/CV signal remapped to 0…5 V |
| Gate outputs | XOR and pattern-clock events |

This option is better when Burl is primarily a source of modulation for a
larger patch. It can preserve the exact -5…+5 V stepped output or make raw PWM
available as audio. It is worse as a self-contained voice because only one
filter response is available at a time.

### Alternate input personality — external-oscillator lab

Either output option can instead map CV 1/2 to oscillator 1/2 modulation, CV 3
to cutoff, and CV 4 to Change. That exposes the current external oscillator
routes but gives up direct resonance and mix modulation. Because those two
routes replace the internal cross-normals rather than add to them, this should
be an alternate firmware mode, not the recommended default.

## Four knobs are enough, with one deliberate simplification

The two oscillator rates, Change, and cutoff are the best four immediate
performance controls. Resonance can safely start at Burl's shipped 62%
default for bring-up, then be tuned toward the retained `00-Burl4`
high-resonance setting; input mix can start fully internal. The four CV jacks
then supply cutoff, Change, resonance, and mix animation without consuming
panel knobs.
The Daisy adapter must assign nonzero fixed modulation depths for those four
routes; Burl's NT defaults deliberately leave all external CV amounts at zero.

If more panel access proves necessary, a later interaction can use **hold the
button as a secondary layer**:

| Normal knobs | Button-held knobs |
| --- | --- |
| Oscillator 1 frequency | Oscillator 1 feedback |
| Oscillator 2 frequency | Oscillator 2 feedback |
| Change | Resonance |
| Cutoff | Stepped-to-cutoff amount |

That is an **interface proposal**, not required for feasibility. A reset on
button release and secondary-layer activation after a hold threshold can avoid
accidental reseeding while editing.

## Porting feasibility

### DSP source reuse

The reusable core is already separated from the NT adapter:

- `src/voice.cpp`, `src/filter.cpp`, and `src/pattern_generator.cpp` depend on
  the project headers, fixed scalar state, `<cmath>`, and integer types—not on
  the disting NT API.
- `src/plugin.cpp` is the NT-specific parameter, bus, and drawing adapter and
  should not be ported.
- The core is C++11, Cortex-M7 hard-float compatible, allocation-free in its
  process path, and already built for an Arm Cortex-M7 target.

A Daisy firmware therefore needs a new thin adapter: initialize
`DaisyPatchSM`, read the four pots/CV/gate controls (using the reconciled
libDaisy `CV_5`…`CV_8` order without another shuffle), scale physical values into
`VoiceInputs`/`VoiceParameters`, call `Voice::process()` once per audio frame,
scale the selected outputs to codec/CV/gate domains, and handle button/toggle
state.

The Patch Submodule codec's typical physical audio range is ±5 V while
libDaisy audio buffers use normalized floating point. A first adapter can use
approximately `input_volts = input_sample * 5` and
`output_sample = output_volts / 5`, but gain and polarity should be measured on
the actual module before calling this calibrated. The current Burl safety limit
can reach ±10 V, so the adapter must clamp or scale before the codec even if
the default patch stays near ordinary Eurorack level.

### Sample rate and block model

- Burl is retained and tested at 32, 44.1, 48, 88.2, and 96 kHz and obtains the
  host sample rate at construction.
- Patch SM supports 48 and 96 kHz and defaults to 48 kHz/24-bit with a
  48-sample callback block in current libDaisy.
- Burl processes one frame at a time and has no fixed block-length assumption.

**Inference:** start at 48 kHz. That is a directly shared, already-tested rate
and leaves the most CPU headroom. Block size can be chosen for reliable audio;
only the optional gate-output timing scheme pressures it toward smaller
blocks.

There is also stronger **local target evidence** for this particular physical
Patch.Init. The sibling `corrupter_patch_init` bring-up record runs the unit at
48 kHz with a 48-sample block and records that a known-good audio firmware hung
when forced to 96 kHz (`README.md`, Hardware configuration). A separate local
firmware has run it at 48 kHz with a 32-sample block. The port should therefore
treat 48 kHz as the hardware baseline, not merely the conservative choice from
the public specification.

### Processor budget

Burl's quality modes perform one, two, or four internal substeps per host
sample. Retained NT hardware measurements for the accepted filter build at
48 kHz were approximately 25% Eco, 44–45% Normal, and 82–84% High for one
instance. Those numbers are **current project bench evidence**, documented in
[the native hardware record](NATIVE_BUILD_AND_HARDWARE.md), but they are not a
Daisy benchmark: firmware overhead, clocking, memory placement, compiler,
math library, and host CPU accounting differ.

The Patch Submodule's official 480 MHz Cortex-M7 specification makes a
single-voice port credible. It does not prove that Normal or High meets a safe
callback deadline. The port should use libDaisy's official
[`CpuLoadMeter`](https://electro-smith.github.io/libDaisy/classdaisy_1_1_cpu_load_meter.html)
around the real callback and record average **and maximum** load:

1. bring up Eco at 48 kHz with audio outputs only;
2. add controls, CV DAC, and gate handling;
3. require comfortable maximum-load margin before enabling Normal;
4. treat High as optional unless the measured worst case remains safe.

The likely hotspot is the per-substep nonlinear math (`pow` in oscillator and
filter frequency calculations), not state movement. Optimization should be
measurement-led and preserve the sound rather than assuming a compiler change
is transparent.

### Memory and program size

Memory is not a concern. Burl declares one `Voice`, uses fixed inline scalar
state, requests no NT DRAM/SDRAM, and has no dynamic-allocation symbol in the
verified Arm object. The current complete NT relocatable object is about 9 KB
by section total. Patch SM officially supplies 64 MB SDRAM in addition to MCU
memory and external program storage. A Daisy adapter and libDaisy runtime will
increase the final firmware size, but the Burl engine itself is tiny relative
to the platform.

That zero-SDRAM design is especially helpful on the user's unit. The retained
`corrupter_patch_init/AGENTS.md` hardware findings show that its SDRAM passes
idle tests but becomes unreliable for audio history while SAI DMA is active;
SRAM audio is clean. Burl needs only small scalar state, so it can remain in
internal SRAM and avoid the already-known unit-specific SDRAM fault entirely.

## What is lost, optional, or changed in v1

| Capability | v1 status | Reason or recovery path |
| --- | --- | --- |
| Core dual-oscillator/pattern/filter sound | Preserved | Reuse the existing core at 48 kHz |
| LP and BP together | Preserved | The two codec outputs |
| HP at the same time | Lost | No third codec audio output; make it selectable later |
| Eight stepped-CV levels | Preserved mathematically | Exact level set/order survives 0…5 V remapping; a block-rate held-DAC implementation can skip fast intermediate transitions |
| Exact bipolar stepped-CV voltage | Changed | Use Option B and a DC-coupled audio output when exact ±5 V is required |
| XOR logic | Preserved as 0/5 V logic | Block-sampled by default; event-accurate transitions require deliberate scheduling |
| Raw audio-rate PWM jack | Optional/lost in recommended v1 | Keep it internal for the filter; Option B can place it on a codec output |
| Aux A/B jacks | Lost | They remain computable but have no dedicated physical outputs |
| External filter audio | Preserved | One or both AC-coupled audio inputs |
| External clock and reset | Preserved | Two gate inputs |
| Cutoff, Change, resonance, and mix CV | Preserved | Four CV inputs |
| Both external oscillator replacement routes | Optional alternate mode | They would consume two of the four CV inputs and replace internal cross-normals |
| All 50 NT parameters immediately editable | Lost by design | Fixed defaults, four-knob performance surface, optional secondary layer/SD settings |
| NT bus routing and Add/Replace mixing | Not applicable | Fixed hardware jacks replace the NT host routing layer |
| Eco/Normal/High quality choice | Eco first; others conditional | Must be profiled on Patch.Init |

## Recommended go/no-go test

The evidence supports a **go for a small porting prototype**, not yet a promise
of a finished firmware image. The shortest decisive experiment is:

1. Build only the three portable Burl core sources plus a minimal
   `DaisyPatchSM` adapter.
2. Run at 48 kHz in Eco and render LP/BP to the codec outputs.
3. Measure maximum callback load with `CpuLoadMeter`; then repeat in Normal.
4. Confirm physical gain, polarity, noise, and the characteristic accepted Burl
   sound on the user's Patch.Init.
5. Add four knobs and the recommended CV/clock/reset map.
6. Add the shifted stepped CV and XOR output first as explicitly labelled
   control-rate/block-sampled signals; verify their real voltages and timing
   with a scope or downstream module.
7. Only if musically necessary, prototype timer/DMA or one-sample scheduling
   for event-accurate pattern clock, XOR, fast stepped CV, or PWM.

If Eco sounds right and Normal has safe worst-case headroom, there is no
remaining architectural blocker. If Normal is too close to the deadline, an
Eco-only v1 would still be a useful four-knob chaotic voice rather than a failed
port.

## Primary sources

- Electrosmith, [Patch.Init product documentation](https://docs.daisy.audio/product/Daisy-Patch-Init/)
- Electrosmith, [Patch.Init databrief](https://daisy.nyc3.cdn.digitaloceanspaces.com/products/patch-init/patch_init_databrief-3-13.pdf)
- Electrosmith, [Patch.Init schematic](https://daisy.nyc3.cdn.digitaloceanspaces.com/products/patch-init/patch_init_schematic.pdf)
- Electrosmith, [Patch Submodule product documentation](https://docs.daisy.audio/hardware/PatchSM/)
- Electrosmith, [Patch Submodule datasheet v1.0.5](https://daisy.nyc3.cdn.digitaloceanspaces.com/products/patch-sm/ES_Patch_SM_datasheet_v1.0.5.pdf)
- Electrosmith, [`DaisyPatchSM` libDaisy header](https://github.com/electro-smith/libDaisy/blob/c02245d22b38acad3916d9c2f156bcba34fa15af/src/daisy_patch_sm.h)
- Electrosmith, [`DaisyPatchSM` libDaisy implementation](https://github.com/electro-smith/libDaisy/blob/c02245d22b38acad3916d9c2f156bcba34fa15af/src/daisy_patch_sm.cpp)
- Electrosmith, [libDaisy bipolar control implementation](https://github.com/electro-smith/libDaisy/blob/c02245d22b38acad3916d9c2f156bcba34fa15af/src/hid/ctrl.cpp)
- Electrosmith, [official Patch.Init hardware-test firmware](https://github.com/electro-smith/DaisyExamples/blob/259ed82c7d4c0d7699f695945e6e111b7dc7cb27/patch_sm/HardwareTest/HardwareTest.cpp)
- Electrosmith, [libDaisy CPU-load meter](https://electro-smith.github.io/libDaisy/classdaisy_1_1_cpu_load_meter.html)
