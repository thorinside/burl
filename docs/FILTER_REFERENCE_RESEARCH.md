# Filter reference research: primary-source findings

## Purpose and source boundary

This note establishes a defensible reference for code-generated filter WAVs.
It uses only source-owned material: Rob Hordijk's 2009 schematic and technical
notes, Hordijk's published filter theory, official licensed-product manuals,
licensed Macumbista instrument documentation, and the original SSM2164
manufacturer's data sheet. It deliberately separates explicit statements from
circuit-derived inference.

The first conclusion is that there is no single circuit called simply "the
filter" for this purpose. The 2020 V2 has explicitly **reworked oscillator and
filter sections** and Eurorack-scaled levels. Its filter input differs from the
2009 standalone circuit. A golden renderer must therefore name its target:

- **2009 standalone/V1 topology**, as drawn in Hordijk's schematic; or
- **2020 V2 behavior**, as described in the V2 manual.

Combining the V1 audio-source mix with V2 input/output behavior is a hybrid,
not a source-faithful model.

Primary sources:

- [Rob Hordijk, original Benjolin schematic (2009), four-page PDF](https://electro-music.com/forum/download.php?id=20221)
- [Hordijk's original filter notes](https://electro-music.com/forum/post-272431.html#272431)
- [Hordijk's identification of the R39 character path](https://electro-music.com/forum/post-272636.html#272636)
- [Hordijk, "The Blippoo Box: A Chaotic Electronic Music Instrument, Bent by Design," DOI 10.1162/lmj.2009.19.35](https://doi.org/10.1162/lmj.2009.19.35)
- [Epoch Modular Benjolin manual (first-party manual mirror)](https://www.manualslib.com/manual/1636627/Epoch-Modular-Benjolin.html)
- [After Later Audio, official Benjolin V2 product page](https://afterlateraudio.com/products/benjolin-v2)
- [Benjolin V2 Manual, Revision B](https://cdn.shopify.com/s/files/1/0591/4309/4430/files/Benjolin_V2_Manual_-_Rev_B_-_Links.pdf?v=1690421061)
- [Macumbista Benjolin V5, an officially licensed Hordijk implementation](https://macumbista.net/?page_id=4770)
- [Analog Devices SSM2164 data sheet](https://www.analog.com/media/en/technical-documentation/data-sheets/ssm2164.pdf)

## Findings

### 1. Filter topology

**Explicit source statement (V1).** Hordijk's original filter notes call the
circuit a plain two-pole state-variable filter "with some added tricks." Two
SSM2164 gain cells drive the integrating op-amps. One of the tricks is a fixed
frequency-control feedback path that gives the resonance an all-harmonic,
skewed character; it is covered separately below.

**Explicit source statement (V2).** The V2 manual calls the VCF a "more or
less standard" voltage-controlled **12 dB state-variable filter** (manual,
pp. 2 and 14). It exposes LP, BP, and HP simultaneously (pp. 7 and 14).

**Schematic-derived inference (V1).** Page 4 of the 2009 schematic shows two
cascaded voltage-controlled integrators. Each uses one SSM2164 gain cell, a
TL074 op-amp, and a 220 pF integrating capacitor (C12 and C13). The second
integrator's output is returned to the U3.D summing stage through R28. This is
the standard two-integrator state-variable structure. By topology, U3.D is the
high-pass/summing node, U3.C is the band-pass integrator output, and U1.D is the
low-pass integrator output. Those HP/BP/LP names are circuit inference; the
three nodes are not labelled that way on the V1 drawing.

The V1 and V2 sources support a conventional two-pole SVF core. For a V1
target, however, the fixed R39 character path is part of the original topology
and cannot be discarded from the final source-faithful render.

### 2. What drives the filter

**Explicit source statement (V2).** The V2 input is an equal-loudness
crossfader between the internal **PWM comparator signal** and `Ext In`. With no
external plug, the control acts as a volume control from silence to internal
PWM (manual, pp. 6, 13-14). The manual is explicit that this is not a crossfade
between the two oscillators. The internal PWM is produced by comparing the two
triangle waves.

**Explicit source statement (licensed V1-derived instrument).** Macumbista's
licensed implementation describes the normal standalone path as PWM "mixed
with a small amount of RUN voltage." This agrees qualitatively with the 2009
schematic.

**Schematic fact and scale limitation (V1).** On page 4, PWM reaches a common
node through R19 = 150 kOhm and RUNCV reaches it through R44 = 51 kOhm. R20 =
10 kOhm shunts that node to analog ground, and the node feeds the filter summer
through R31 = 100 kOhm. This proves that both signals enter the V1 path.
However, it does **not** prove a normalized formula such as
`PWM + 0.10 * RUNCV`: the two source voltages have different native ranges and
offsets, and the node has additional loading. For equal ideal source voltages,
the conductance ratio alone would actually weight RUNCV about 2.94 times PWM.
An exact normalized mix therefore requires a component-level circuit model or
measurements of the schematic's native PWM and RUNCV levels.

The Epoch manual supplies a useful level constraint for its licensed V1
implementation: PWM is approximately +/-8 V, Rungler approximately +/-5.5 V,
and the filter outputs only approximately +/-1.5 V. It also explicitly calls
the filter input a PWM/Rungler mix. Those levels are not enough to solve every
internal node, but they demonstrate that a unity-scaled +/-5 V PWM driven
directly into a unity-scaled digital SVF is not equivalent to this analog
gain structure.

By contrast, the V2 manual specifies all front-panel connector outputs in the
+/-5 V Eurorack range (p. 22). This is another reason not to use the V1's
front-panel levels and the V2's output normalization in one unnamed model.

**Defensible ideal-op-amp estimate (V1).** The published resistor network does
support a useful zero-delay feed estimate. Let `p` be PWM voltage, `r` be
RUNCV voltage, `b` be the first-pole/BP voltage, and `s` be the passive common
node feeding R31 and R38. Treating the op-amp summing inputs as ideal gives:

```text
s = (p/150k + r/51k + b/100k)
    / (1/150k + 1/51k + 1/10k + 1/100k + 1/100k)
  = 0.04558 p + 0.13405 r + 0.06836 b
```

U3.D has R30 = 30 kOhm feedback, receives `s` through R31 = 100 kOhm,
and receives the second-pole/LP feedback through R28 = 30 kOhm. Before the
separate P5 resonance path is solved, its summing output is therefore:

```text
h = -0.3 s - LP
  = -0.01367 p - 0.04021 r - 0.02051 b - LP
```

This is not yet a complete filter transfer function; P5/R37/R35, U1.C, the
two integrators, R39, VCA control law, and output buffer remain in the loop.
It is nevertheless enough to reject unity input scaling for a V1 model. With
the Epoch manual's +/-8 V PWM, a zero-state PWM edge contributes only about
0.109 V at this summer, whereas feeding +/-5 V directly to a unity-input SVF
contributes +/-5 V: about 46 times the initial forcing. If both original
sources happen to be at their same-sign maxima, their direct terms sum to
about 0.33 V; that is still far below a unity-scaled `5 + 0.10 * 5.5` V input.

The +/-1.5 V Epoch filter-output figure must **not** be divided by +/-8 V to
make a universal 0.1875 gain. Resonant gain depends on cutoff, damping,
stimulus spectrum, selected output, and the feedback network. The correct next
step is a SPICE/netlist solution of page 4 using the published native levels,
followed by a separately justified output normalization.

Consequently:

- a V2 reference render should use **PWM only** as the internal source;
- a V1 reference render should reproduce the passive resistor network and
  native source levels, not assume a 10% normalized addition; and
- `PWM + 0.10 * stepped CV` was an initial project choice, not a
  primary-source result, and was rejected after the static diagnosis.

### 3. Resonance, pinging, and self-oscillation

**Explicit source statement.** The V2 manual says the filter has exceptionally
good pinging characteristics. Maximum normal resonance is **on the brink of
oscillation**. A sharp LFO pulse or waveform flank produces a short sine burst
that dies out "after a second or so"; using BP removes the initial flank and
gives the cleanest burst (manual, pp. 2 and 14).

This is the documented source of the bongo-like sound: both oscillators are
tuned into the LFO range, their PWM comparator creates irregular pulse flanks,
and those flanks excite a nearly undamped filter. The manual explicitly calls
the result a percussive rhythm (p. 13). It is not described as broadband noise
or a continuously running internal oscillator.

**Explicit source statement about sustained oscillation.** To make the V2
filter a continuous sine oscillator, the manual instructs the user to patch BP
back to `Ext In` and select that external input (pp. 9 and 14-15). At larger
resonance settings that externally closed loop clips the sine tops. Thus:

- the normal internal path should ring strongly but lose energy;
- silence should remain silent;
- continuous self-oscillation belongs to an explicit BP-to-input feedback
  patch, not the unpatched baseline; and
- BP is the decisive ping reference because its output omits the exciting
  edge.

No primary source found here gives an absolute Q, a knob-to-Q equation, or a
60 dB decay-time curve. In particular, `Q = 0.5 * 40^res`, Q = 20, and a 174 ms
maximum-resonance -60 dB tail are not source-derived. The documented
"second or so" tail suggests that 174 ms should not be accepted as proven
without reconciling stimulus amplitude, cutoff, output, and the manual's
informal audibility threshold.

### 4. Modulation and nonlinear character

**Explicit source statement (V1).** Hordijk says a single-resistor trick uses
the sine/cosine relationship between the two pole outputs to build
all-harmonic, "tube-like" distortion. It skews a sine-like resonance toward a
sawtooth-like waveform. In a follow-up he identifies the part unambiguously as
R39 = 100 kOhm from the output of the first pole to the frequency-modulation
input. The Epoch manual calls the same feature Z-plane modulation and says its
effect is most palpable near maximum resonance with both oscillators at audio
rate.

**Schematic-derived magnitude estimate (V1).** Page 4 connects U3.C pin 8, the
first integrator output, through fixed R39 = 100 kOhm to U5.A's cutoff-control
summer. U5.A has R45 = 10 kOhm in feedback, so the path contributes roughly
one tenth of the first-pole voltage to the SSM2164 control voltage. The
SSM2164 data sheet specifies about 33 mV/dB. Using 6.02 dB per octave, the
small-signal magnitude works out to approximately **0.50 cutoff octave per
volt of first-pole output**, before the circuit's CV limiting and headroom are
considered. This is an inference, not a published calibration, but it is a
better starting point for SPICE and high-rate renders than a normalized clamp.
If the Epoch manual's roughly +/-1.5 V filter-output range also approximates
the U3.C node range, the implied character modulation is roughly +/-0.75
octave. That last step is conditional because the manual specifies connector
levels, not the internal R39 node.

R39 is fixed. It is not multiplied by the resonance control. The audible skew
grows with resonance because the first-pole signal itself grows. Hordijk's
published theory also warns that excessive nonlinear frequency feedback
becomes input-level-dependent and can enter chaotic behavior.

**Explicit source statement (V2 and user-patched modulation).** Hordijk's V2 description says cutoff accepts
deep modulation at high audio rates. For a percussive PWM patch, frequency
modulation changes timbre. For a clean ping, this modulation is optional, not
part of the core filter. The manual separately describes patching HP to the
frequency-CV input to morph a self-oscillating sine toward a bright sawtooth,
with detuning as a tradeoff (pp. 14-15).

The V2 manual does not disclose whether the reworked V2 circuit retains R39,
so its optional HP patch must not be used as proof either way. For V1, the
fixed character path is explicit. What the sources do **not** support is the
formula `0.5 * resonance^2 * clamp(previousBand / 5)` or another explicitly
resonance-gated, clamped substitute.

**Explicit source statement (V1 resonance contour).** A relatively small
resistor across the linear resonance pot makes its response slightly anti-log.
Hordijk also says that increasing resonance subtracts some of the filter input
through that pot to balance output level. Page 4 shows P5 = 10 kOhm and R37 =
1.5 kOhm in this network. This validates resonance-linked input compensation
as a concept, but not the particular digital curve `1 - 0.25 * resonance^2`;
the actual pot/resistor network must be solved.

**Schematic-derived and component-derived finding.** The V1 audio path around
the two integrators contains no diode waveshaper or intentional audio clipper.
The two diodes around U5.A are in the **frequency-control** summing path. The
SSM2164 data sheet specifies 0.02% typical THD and a +22 dBu 1%-THD clip point
in its stated test circuit, so the gain cells are intended to be linear over
normal signal levels. Real op-amp/gain-cell headroom eventually produces the
clipping noted by the manual in the externally self-oscillating patch, but the
sources do not document a pre-filter soft saturator, the particular quadratic
input-compensation curve currently proposed, or an `Input Drive` control.

Those project-specific extensions must be disabled in the core oracle and
tested separately. A V1-complete render should then add the solved original
resonance-linked input-balancing network, not an assumed quadratic substitute.

### 5. Outputs and DC coupling

**Explicit source statement (V2).** V2 exposes LP, BP, and HP. Their phase
relationship is described as LP 0 degrees, BP 90 degrees, and HP 0 degrees;
the manual warns that LP and HP are not in antiphase, so one must be inverted
before summing them into a notch (p. 14).

**Schematic-derived inference (V1).** The V1 filter core has no series coupling
capacitor at its input or state outputs. C12 and C13 are feedback capacitors
that form the integrators, not DC blockers. The drawing therefore supports a
DC-coupled mathematical model: LP can retain input offset while ideal stable
BP and HP settle toward zero. The V2 circuit was reworked and its schematic is
not in the cited materials, so V1 DC coupling must not be presented as an
explicit V2 specification.

The original standalone connector labels a single `OUT`; the V2 manual is the
source that explicitly promises three separately available outputs.

## Why a digital render can sound like static

The sources identify several ways to distinguish intended roughness from a
bug. These are engineering inferences from the documented signal path, not
quotes from the designer.

1. **Wrong test patch.** With both oscillators at audio rate, the comparator is
   a discontinuous, harmonically dense PWM signal. The manual says it moves
   toward a ring-modulation-like sound. That is a poor stimulus for judging a
   clean bongo ping. The documented ping patch puts both oscillators in the LFO
   range and listens primarily to BP.
2. **Digital aliasing.** A sample-by-sample hard comparator creates harmonics
   above Nyquist. An analog circuit does not fold those harmonics back into the
   audio band before its continuous-time filter; a naive digital model does.
   The golden render must run much faster than delivery rate and use a defined
   low-pass downsampler.
3. **Stepped-CV injection.** Adding normalized stepped CV directly to audio
   creates discontinuities on Rungler changes. A wrong scale can turn a "small
   amount" into repeated clicks, each of which pings the filter. V2 should omit
   this audio injection entirely.
4. **Mis-scaled character feedback.** V1 genuinely has fixed first-pole-to-FCV
   feedback, but a sample-delayed, resonance-squared and hard-clamped
   approximation is not the drawn network. Wrong signal level, delay, or
   alias control can turn the intended skew into unstable or static-like FM.
   A V2-only oracle must treat this path as unknown until its reworked circuit
   or a controlled behavioral fit is available.
5. **Numerical or limiter chatter.** A correctly damped filter driven by
   silence cannot create a wideband floor. Static that remains with PWM,
   Rungler audio injection, drive, limiter, and cutoff modulation removed is a
   numerical defect, not documented character.

The first candidate's reported +/-5 V near-unity filter drive, +/-8 to +/-10 V
filter outputs, and 18.7% output-limiter occupancy were consistent with the
overdrive hypothesis above. They are implementation observations, not
historical-source claims. The V1 correction now uses the resistor-derived
forcing terms and is evaluated separately from any V2 model.

## Proposed theoretical WAV reference set

Generate the reference in double precision at no less than 16 times the final
sample rate, then low-pass and decimate with a recorded FIR specification.
Keep the **SVF-core oracle** linear: no input-drive saturation, output limiter,
resonance compensation, or character feedback. Then add separately named V1
stages for the solved passive input/resonance network and fixed R39 feedback.
This separation lets residuals identify the stage that creates an error. Export
32-bit float WAV plus a sidecar file containing exact parameters, renderer
commit, peak, RMS, and spectral metrics.

Render these two named topology families rather than one hybrid:

| Fixture | Source and controls | Expected evidence |
|---|---|---|
| V2 silence | Zero input; resonance min, mid, max | Exact silence; no free oscillation |
| V2 impulse/step | One sharp edge; fixed 250 Hz cutoff; resonance sweep | Damped sine burst; longest tail near max |
| V2 clean ping | Both triangles at distinct LFO rates; PWM only; max resonance; cutoff CV closed | Irregular bongo-like pulses; BP omits the initial edge |
| V2 dense PWM | Two incommensurate audio-rate triangles; PWM only | Intentionally bright/ring-mod-like, but no unexplained stationary noise floor |
| V2 external sine | Low-level swept sine; resonance sweep | Linear transfer, simultaneous LP/BP/HP phase and magnitude checks |
| V2 external BP loop | BP returned to Ext In as the manual instructs | Sustained sine; clipping only as loop gain/resonance is opened further |
| V1 clean ping | Same LFO comparator plus the component-level PWM/RUNCV passive mix | Difference from V2 attributable to the documented RUN and level network |
| V1 character A/B | Solved V1 levels, first with R39 open and then with fixed R39 = 100 kOhm | Isolates all-harmonic skew, aliasing, and stability limits |
| V1/V2 DC step | Constant external voltage, where supported | LP retains offset; BP/HP settle toward zero in the DC-coupled model |

For every fixture, also render the same logical patch at 44.1, 48, 88.2, 96,
and 192 kHz from the high-rate master. Compare the production renderer to the
downsampled master using time-aligned residual RMS, peak error, decay envelope,
and in-band spectral error. A production-quality change should reduce those
errors across rates rather than merely match one hand-selected waveform.

## Implementation implications

The strongest source-backed model is more specific than the current character
hypothesis:

- a conventional 12 dB SVF;
- PWM-only internal audio for a V2 target, or a component-level PWM/RUNCV mix
  for a V1 target;
- resonance implemented as decreasing damping, reaching the brink of
  oscillation at maximum but still decaying after excitation;
- for V1, a fixed 100 kOhm first-pole-to-frequency-control path whose depth
  follows the pole amplitude naturally, plus the drawn anti-log resonance and
  level-balancing network;
- for V2, no assumption about retained V1 character circuitry beyond the
  behaviors the reworked V2 manual actually documents;
- no intentional input saturation at ordinary levels; and
- LP/BP/HP observed separately, with BP as the primary clean-ping test.

The 10% normalized Rungler addition, resonance-squared character depth, and
character clamp were rejected by this pass. The numerical Q map, quadratic
input compensation, soft saturation, and output normalization still must be
treated as hypotheses to A/B against the source-backed baseline, not as
established properties of the original circuit. The implemented 16x candidate
oracle and its limitations are recorded in
`docs/THEORETICAL_AUDIO_VERIFICATION.md`.
