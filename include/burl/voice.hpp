// Copyright (c) 2026 Burl contributors
// SPDX-License-Identifier: MIT

#ifndef BURL_VOICE_HPP
#define BURL_VOICE_HPP

#include "burl/pattern_generator.hpp"

#include <stdint.h>

namespace burl {

enum QualityMode {
    QualityEco,
    QualityNormal,
    QualityHigh
};

/** Host-rate controls for the self-contained Burl DSP voice. */
struct VoiceParameters {
    float oscillator1Hz;
    float oscillator2Hz;
    float oscillator1CrossModulation;
    float oscillator2CrossModulation;
    float oscillator1Feedback;
    float oscillator2Feedback;
    float change;
    float filterCutoffHz;
    float filterResonance;
    float filterFeedback;
    float externalCutoffModulation;
    float externalInputMix;
    bool useExternalOscillator1Cv;
    bool useExternalOscillator2Cv;
    bool useExternalClock;
    bool doubleEdgeClock;
    bool maximal127Mode;
    bool safetyLimit;
    QualityMode quality;
    unsigned int dacMsbTap;
    unsigned int dacMiddleTap;
    unsigned int dacLsbTap;

    VoiceParameters();
};

/** Conditioned input values supplied by the eventual disting NT adapter. */
struct VoiceInputs {
    float oscillator1Cv;
    float oscillator2Cv;
    float externalClock;
    float filterAudio;
    float cutoffCv;

    VoiceInputs();
};

/** The eight independently routable Burl signals, expressed in volts. */
struct VoiceOutputs {
    float lowPass;
    float bandPass;
    float highPass;
    float steppedCv;
    float pwm;
    float registerXor;
    float auxA;
    float auxB;

    VoiceOutputs();
};

/** Small bounded status suitable for tests and the later minimal draw view. */
struct VoiceStatus {
    float oscillator1Triangle;
    float oscillator2Triangle;
    uint8_t patternState;
    bool active;
};

/**
 * Deterministic, allocation-free DSP core for one Burl voice.
 *
 * process() consumes and produces one host-rate frame. Closely coupled DSP is
 * evaluated at the quality mode's common internal rate and boxcar low-pass
 * decimated to the host rate. Runtime quality changes alter only the number of
 * preallocated scalar substeps; they do not reconstruct this object.
 */
class Voice {
public:
    explicit Voice(float sampleRate, uint8_t seed = 1u);

    void setSampleRate(float sampleRate);
    float sampleRate() const;

    void setParameters(const VoiceParameters& parameters);
    const VoiceParameters& parameters() const;

    void setSeed(uint8_t seed);
    uint8_t seed() const;
    void reset();

    VoiceOutputs process(const VoiceInputs& inputs);
    VoiceStatus status() const;

private:
    struct OscillatorState {
        float triangle;
        float direction;
    };

    struct FilterState {
        float integrator1;
        float integrator2;
    };

    static float clamp(float value, float low, float high);
    static float finiteClamp(float value, float limit);
    static unsigned int qualityFactor(QualityMode quality);
    static float advanceOscillator(OscillatorState& oscillator, float frequency,
                                   float internalSampleRate);

    bool clockEdge(float internalPulse, float externalClock);
    VoiceOutputs processInternal(const VoiceInputs& inputs,
                                 float internalSampleRate);

    float sampleRate_;
    uint8_t configuredSeed_;
    VoiceParameters parameters_;
    PatternGenerator pattern_;
    OscillatorState oscillator1_;
    OscillatorState oscillator2_;
    FilterState filter_;
    float steppedCv_;
    bool previousInternalPulseHigh_;
    bool externalClockHigh_;
    VoiceStatus status_;
};

} // namespace burl

#endif // BURL_VOICE_HPP
