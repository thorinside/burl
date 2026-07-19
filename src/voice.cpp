// Copyright (c) 2026 Burl contributors
// SPDX-License-Identifier: MIT

#include "burl/voice.hpp"

#include <cmath>

namespace burl {
namespace {

const float kPi = 3.14159265358979323846f;

} // namespace

VoiceParameters::VoiceParameters()
    : oscillator1Hz(110.0f),
      oscillator2Hz(173.0f),
      oscillator1CrossModulation(1.0f),
      oscillator2CrossModulation(1.0f),
      oscillator1Feedback(0.5f),
      oscillator2Feedback(-0.5f),
      change(0.5f),
      filterCutoffHz(1200.0f),
      filterResonance(0.5f),
      filterFeedback(0.5f),
      externalCutoffModulation(0.0f),
      externalInputMix(0.0f),
      changeCvAmount(0.0f),
      resonanceCvAmount(0.0f),
      mixCvAmount(0.0f),
      inputDrive(1.0f),
      useExternalOscillator1Cv(false),
      useExternalOscillator2Cv(false),
      useExternalClock(false),
      doubleEdgeClock(false),
      maximal127Mode(false),
      safetyLimit(true),
      quality(QualityNormal),
      dacMsbTap(7u),
      dacMiddleTap(5u),
      dacLsbTap(3u),
      auxASource(AuxOscillator1Triangle),
      auxBSource(AuxOscillator2Triangle) {}

VoiceInputs::VoiceInputs()
    : oscillator1Cv(0.0f), oscillator2Cv(0.0f), externalClock(0.0f),
      filterAudio(0.0f), cutoffCv(0.0f), changeCv(0.0f),
      resonanceCv(0.0f), mixCv(0.0f), reset(0.0f) {}

VoiceOutputs::VoiceOutputs()
    : lowPass(0.0f), bandPass(0.0f), highPass(0.0f), steppedCv(0.0f),
      pwm(0.0f), registerXor(0.0f), auxA(0.0f), auxB(0.0f) {}

Voice::Voice(float sampleRate, uint8_t seed)
    : sampleRate_(48000.0f),
      configuredSeed_(seed),
      parameters_(),
      pattern_(seed),
      oscillator1_(),
      oscillator2_(),
      filter_(),
      steppedCv_(0.0f),
      previousInternalPulseHigh_(false),
      externalClockHigh_(false),
      resetInputHigh_(false),
      status_() {
    setSampleRate(sampleRate);
    reset();
}

void Voice::setSampleRate(float newSampleRate) {
    sampleRate_ = std::isfinite(newSampleRate) && newSampleRate > 0.0f
        ? newSampleRate
        : 48000.0f;
}

float Voice::sampleRate() const {
    return sampleRate_;
}

void Voice::setParameters(const VoiceParameters& newParameters) {
    parameters_ = newParameters;
    const PatternGenerator::Mode requestedMode = parameters_.maximal127Mode
        ? PatternGenerator::Maximal127
        : PatternGenerator::EightSixteen;
    if (pattern_.mode() != requestedMode) {
        pattern_.setMode(requestedMode);
    }
    steppedCv_ = pattern_.dac(parameters_.dacMsbTap,
                              parameters_.dacMiddleTap,
                              parameters_.dacLsbTap);
}

const VoiceParameters& Voice::parameters() const {
    return parameters_;
}

void Voice::setSeed(uint8_t newSeed) {
    configuredSeed_ = newSeed;
    pattern_.setSeed(newSeed);
}

uint8_t Voice::seed() const {
    return configuredSeed_;
}

void Voice::reset() {
    pattern_.setSeed(configuredSeed_);
    pattern_.setMode(parameters_.maximal127Mode
                         ? PatternGenerator::Maximal127
                         : PatternGenerator::EightSixteen);
    pattern_.reset();

    oscillator1_.triangle = -0.5f;
    oscillator1_.direction = 1.0f;
    oscillator2_.triangle = 0.25f;
    oscillator2_.direction = -1.0f;
    filter_.integrator1 = 0.0f;
    filter_.integrator2 = 0.0f;
    steppedCv_ = pattern_.dac(parameters_.dacMsbTap,
                              parameters_.dacMiddleTap,
                              parameters_.dacLsbTap);
    previousInternalPulseHigh_ = oscillator2_.direction > 0.0f;
    externalClockHigh_ = false;
    resetInputHigh_ = false;

    status_.oscillator1Triangle = oscillator1_.triangle * 5.0f;
    status_.oscillator2Triangle = oscillator2_.triangle * 5.0f;
    status_.steppedCv = steppedCv_;
    status_.patternState = pattern_.state();
    status_.externalClock = parameters_.useExternalClock;
    status_.active = false;
}

VoiceOutputs Voice::process(const VoiceInputs& inputs) {
    if (resetEdge(inputs.reset)) {
        const bool resetInputHigh = resetInputHigh_;
        reset();
        resetInputHigh_ = resetInputHigh;
    }

    VoiceOutputs accumulated;
    const unsigned int factor = qualityFactor(parameters_.quality);
    const float internalSampleRate = sampleRate_ * static_cast<float>(factor);

    for (unsigned int substep = 0u; substep < factor; ++substep) {
        const VoiceOutputs current = processInternal(inputs, internalSampleRate);
        accumulated.lowPass += current.lowPass;
        accumulated.bandPass += current.bandPass;
        accumulated.highPass += current.highPass;
        accumulated.steppedCv += current.steppedCv;
        accumulated.pwm += current.pwm;
        accumulated.registerXor += current.registerXor;
        accumulated.auxA += current.auxA;
        accumulated.auxB += current.auxB;
    }

    const float inverseFactor = 1.0f / static_cast<float>(factor);
    const float outputLimit = parameters_.safetyLimit ? 10.0f : 100.0f;
    accumulated.lowPass = finiteClamp(accumulated.lowPass * inverseFactor,
                                      outputLimit);
    accumulated.bandPass = finiteClamp(accumulated.bandPass * inverseFactor,
                                       outputLimit);
    accumulated.highPass = finiteClamp(accumulated.highPass * inverseFactor,
                                       outputLimit);
    accumulated.steppedCv = finiteClamp(accumulated.steppedCv * inverseFactor,
                                        outputLimit);
    accumulated.pwm = finiteClamp(accumulated.pwm * inverseFactor, outputLimit);
    accumulated.registerXor = finiteClamp(
        accumulated.registerXor * inverseFactor, outputLimit);
    accumulated.auxA = finiteClamp(accumulated.auxA * inverseFactor, outputLimit);
    accumulated.auxB = finiteClamp(accumulated.auxB * inverseFactor, outputLimit);

    status_.oscillator1Triangle = oscillator1_.triangle * 5.0f;
    status_.oscillator2Triangle = oscillator2_.triangle * 5.0f;
    status_.steppedCv = steppedCv_;
    status_.patternState = pattern_.state();
    status_.externalClock = parameters_.useExternalClock;
    status_.active = true;
    return accumulated;
}

VoiceStatus Voice::status() const {
    return status_;
}

float Voice::clamp(float value, float low, float high) {
    return value < low ? low : (value > high ? high : value);
}

float Voice::finiteClamp(float value, float limit) {
    if (!std::isfinite(value)) {
        return 0.0f;
    }
    return clamp(value, -limit, limit);
}

unsigned int Voice::qualityFactor(QualityMode quality) {
    switch (quality) {
    case QualityEco:
        return 1u;
    case QualityNormal:
        return 2u;
    case QualityHigh:
        return 4u;
    }
    return 2u;
}

float Voice::advanceOscillator(OscillatorState& oscillator, float frequency,
                               float internalSampleRate) {
    const float boundedFrequency = clamp(frequency, 0.001f,
                                         internalSampleRate * 0.45f);
    const float distance = 4.0f * boundedFrequency / internalSampleRate;
    oscillator.triangle += oscillator.direction * distance;

    while (oscillator.triangle > 1.0f || oscillator.triangle < -1.0f) {
        if (oscillator.triangle > 1.0f) {
            oscillator.triangle = 2.0f - oscillator.triangle;
            oscillator.direction = -1.0f;
        } else {
            oscillator.triangle = -2.0f - oscillator.triangle;
            oscillator.direction = 1.0f;
        }
    }

    return oscillator.triangle;
}

float Voice::selectAux(AuxSource source, float oscillator1Triangle,
                       float oscillator1Pulse, float oscillator2Triangle,
                       float oscillator2Pulse, const VoiceOutputs& outputs) {
    switch (source) {
    case AuxOscillator1Triangle:
        return oscillator1Triangle;
    case AuxOscillator1Pulse:
        return oscillator1Pulse;
    case AuxOscillator2Triangle:
        return oscillator2Triangle;
    case AuxOscillator2Pulse:
        return oscillator2Pulse;
    case AuxPwm:
        return outputs.pwm;
    case AuxRegisterXor:
        return outputs.registerXor;
    case AuxSteppedCv:
        return outputs.steppedCv;
    case AuxLowPass:
        return outputs.lowPass;
    case AuxBandPass:
        return outputs.bandPass;
    case AuxHighPass:
        return outputs.highPass;
    }
    return 0.0f;
}

bool Voice::clockEdge(float internalPulse, float externalClock) {
    if (!parameters_.useExternalClock) {
        const bool pulseHigh = internalPulse > 0.0f;
        const bool changed = pulseHigh != previousInternalPulseHigh_;
        const bool rising = pulseHigh && !previousInternalPulseHigh_;
        previousInternalPulseHigh_ = pulseHigh;
        return parameters_.doubleEdgeClock ? changed : rising;
    }

    const bool wasHigh = externalClockHigh_;
    const float conditionedClock = finiteClamp(externalClock, 12.0f);
    if (!externalClockHigh_ && conditionedClock >= 2.0f) {
        externalClockHigh_ = true;
    } else if (externalClockHigh_ && conditionedClock <= 0.2f) {
        externalClockHigh_ = false;
    }

    const bool changed = externalClockHigh_ != wasHigh;
    const bool rising = externalClockHigh_ && !wasHigh;
    return parameters_.doubleEdgeClock ? changed : rising;
}

bool Voice::resetEdge(float resetInput) {
    const float conditionedReset = finiteClamp(resetInput, 12.0f);
    const bool wasHigh = resetInputHigh_;
    if (!resetInputHigh_ && conditionedReset >= 2.0f) {
        resetInputHigh_ = true;
    } else if (resetInputHigh_ && conditionedReset <= 0.2f) {
        resetInputHigh_ = false;
    }
    return resetInputHigh_ && !wasHigh;
}

VoiceOutputs Voice::processInternal(const VoiceInputs& inputs,
                                    float internalSampleRate) {
    const float previousSteppedCv = steppedCv_;
    const float normal1 = oscillator2_.triangle;
    const float normal2 = oscillator1_.triangle;
    const float external1 = finiteClamp(inputs.oscillator1Cv, 10.0f) / 5.0f;
    const float external2 = finiteClamp(inputs.oscillator2Cv, 10.0f) / 5.0f;
    const float modulation1 = parameters_.useExternalOscillator1Cv
        ? external1
        : normal1;
    const float modulation2 = parameters_.useExternalOscillator2Cv
        ? external2
        : normal2;

    const float octave1 = clamp(
        parameters_.oscillator1CrossModulation * modulation1
            + parameters_.oscillator1Feedback * (previousSteppedCv / 5.0f),
        -16.0f, 16.0f);
    const float octave2 = clamp(
        parameters_.oscillator2CrossModulation * modulation2
            + parameters_.oscillator2Feedback * (previousSteppedCv / 5.0f),
        -16.0f, 16.0f);
    const float frequency1 = clamp(parameters_.oscillator1Hz, 0.001f,
                                   internalSampleRate * 0.45f)
        * std::pow(2.0f, octave1);
    const float frequency2 = clamp(parameters_.oscillator2Hz, 0.001f,
                                   internalSampleRate * 0.45f)
        * std::pow(2.0f, octave2);

    const float triangle1 = advanceOscillator(oscillator1_, frequency1,
                                               internalSampleRate);
    const float triangle2 = advanceOscillator(oscillator2_, frequency2,
                                               internalSampleRate);
    const float internalPulse = oscillator2_.direction > 0.0f ? 5.0f : -5.0f;

    const float internalAudio = 2.5f * (triangle1 + triangle2);
    const float inputMix = clamp(
        parameters_.externalInputMix
            + parameters_.mixCvAmount
                * (finiteClamp(inputs.mixCv, 10.0f) / 5.0f),
        0.0f, 1.0f);
    const float filterInput = internalAudio * (1.0f - inputMix)
        + finiteClamp(inputs.filterAudio, 12.0f) * inputMix;
    const float drivenFilterInput = filterInput
        * clamp(parameters_.inputDrive, 0.25f, 4.0f);
    const float cutoffOctaves = clamp(
        parameters_.filterFeedback * (previousSteppedCv / 5.0f)
            + parameters_.externalCutoffModulation
                * (finiteClamp(inputs.cutoffCv, 10.0f) / 5.0f),
        -16.0f, 16.0f);
    const float cutoff = clamp(
        clamp(parameters_.filterCutoffHz, 1.0f, internalSampleRate * 0.45f)
            * std::pow(2.0f, cutoffOctaves),
        1.0f, internalSampleRate * 0.45f);
    const float g = std::tan(kPi * cutoff / internalSampleRate);
    const float resonance = clamp(
        parameters_.filterResonance
            + parameters_.resonanceCvAmount
                * (finiteClamp(inputs.resonanceCv, 10.0f) / 5.0f),
        0.0f, 1.0f);
    const float damping = 2.0f - 1.95f * resonance;
    const float a1 = 1.0f / (1.0f + g * (g + damping));
    const float a2 = g * a1;
    const float a3 = g * a2;
    const float v3 = drivenFilterInput - filter_.integrator2;
    const float bandPass = a1 * filter_.integrator1 + a2 * v3;
    const float lowPass = filter_.integrator2
        + a2 * filter_.integrator1 + a3 * v3;
    const float highPass = drivenFilterInput - damping * bandPass - lowPass;
    filter_.integrator1 = finiteClamp(
        2.0f * bandPass - filter_.integrator1, 100.0f);
    filter_.integrator2 = finiteClamp(
        2.0f * lowPass - filter_.integrator2, 100.0f);

    VoiceOutputs output;
    output.lowPass = lowPass;
    output.bandPass = bandPass;
    output.highPass = highPass;
    output.steppedCv = previousSteppedCv;
    output.pwm = triangle2 > triangle1 ? 5.0f : -5.0f;
    output.registerXor = pattern_.bit(0u) != pattern_.bit(7u) ? 5.0f : -5.0f;
    const float oscillator1Pulse = oscillator1_.direction > 0.0f ? 5.0f : -5.0f;
    const float oscillator2Pulse = oscillator2_.direction > 0.0f ? 5.0f : -5.0f;
    output.auxA = selectAux(parameters_.auxASource, triangle1 * 5.0f,
                            oscillator1Pulse, triangle2 * 5.0f,
                            oscillator2Pulse, output);
    output.auxB = selectAux(parameters_.auxBSource, triangle1 * 5.0f,
                            oscillator1Pulse, triangle2 * 5.0f,
                            oscillator2Pulse, output);

    if (clockEdge(internalPulse, inputs.externalClock)) {
        const float comparator = clamp(triangle2 - triangle1, -1.0f, 1.0f);
        const float change = clamp(
            parameters_.change
                + 0.5f * parameters_.changeCvAmount
                    * (finiteClamp(inputs.changeCv, 10.0f) / 5.0f),
            0.0f, 1.0f);
        pattern_.clock(change, comparator);
        steppedCv_ = pattern_.dac(parameters_.dacMsbTap,
                                  parameters_.dacMiddleTap,
                                  parameters_.dacLsbTap);
    }

    return output;
}

} // namespace burl
