// Copyright (c) 2026 Burl contributors
// SPDX-License-Identifier: MIT

#ifndef BURL_VOICE_HPP
#define BURL_VOICE_HPP

#include "burl/filter.hpp"
#include "burl/pattern_generator.hpp"

#include <stdint.h>

namespace burl {

enum QualityMode {
    QualityEco,
    QualityNormal,
    QualityHigh
};

/** Signals available to either selectable auxiliary output. */
enum AuxSource {
    AuxOscillator1Triangle,
    AuxOscillator1Pulse,
    AuxOscillator2Triangle,
    AuxOscillator2Pulse,
    AuxPwm,
    AuxRegisterXor,
    AuxSteppedCv,
    AuxLowPass,
    AuxBandPass,
    AuxHighPass
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
    float changeCvAmount;
    float resonanceCvAmount;
    float mixCvAmount;
    float inputDrive;
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
    AuxSource auxASource;
    AuxSource auxBSource;

    VoiceParameters();
};

/** Conditioned input values supplied by the eventual disting NT adapter. */
struct VoiceInputs {
    float oscillator1Cv;
    float oscillator2Cv;
    float externalClock;
    float filterAudio;
    float cutoffCv;
    float changeCv;
    float resonanceCv;
    float mixCv;
    float reset;

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
    float steppedCv;
    uint8_t patternState;
    bool externalClock;
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

    void setSampleRate(float newSampleRate);
    float sampleRate() const;

    /**
     * Apply host-rate controls without resetting running DSP state.
     * Non-finite floating controls remain bounded during processing, and the
     * voice resumes normally when finite controls are restored.
     */
    void setParameters(const VoiceParameters& newParameters);
    const VoiceParameters& parameters() const;

    void setSeed(uint8_t newSeed);
    uint8_t seed() const;
    void reset();

    VoiceOutputs process(const VoiceInputs& inputs);
    VoiceStatus status() const;

private:
    struct OscillatorState {
        float triangle;
        float direction;
    };

    static float clamp(float value, float low, float high);
    static float finiteClamp(float value, float limit);
    static unsigned int qualityFactor(QualityMode quality);
    static float advanceOscillator(OscillatorState& oscillator, float frequency,
                                   float internalSampleRate);
    static float selectAux(AuxSource source, float oscillator1Triangle,
                           float oscillator1Pulse, float oscillator2Triangle,
                           float oscillator2Pulse, const VoiceOutputs& outputs);

    bool clockEdge(float internalPulse, float externalClock);
    bool resetEdge(float resetInput);
    VoiceOutputs processInternal(const VoiceInputs& inputs,
                                 float internalSampleRate);

    float sampleRate_;
    uint8_t configuredSeed_;
    VoiceParameters parameters_;
    PatternGenerator pattern_;
    OscillatorState oscillator1_;
    OscillatorState oscillator2_;
    StateVariableFilter filter_;
    float steppedCv_;
    bool previousInternalPulseHigh_;
    bool externalClockHigh_;
    bool resetInputHigh_;
    VoiceStatus status_;
};

} // namespace burl

#endif // BURL_VOICE_HPP
