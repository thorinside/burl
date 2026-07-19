// Copyright (c) 2026 Burl contributors
// SPDX-License-Identifier: MIT

#include "burl/filter.hpp"

#include <cmath>

namespace burl {
namespace {

const float kPi = 3.14159265358979323846f;
const float kCharacterSkewOctavesPerVolt = 0.5f;
const float kResonanceInputCompensation = 0.25f;
const unsigned int kResonanceTableSegments = 64u;

// Samples of 1/Q = 2 * 40^-resonance. Linear interpolation keeps audio-rate
// resonance CV inexpensive while retaining the specified exponential curve.
const float kResonanceDampingTable[kResonanceTableSegments + 1u] = {
    2.000000000f,
    1.887981821f,
    1.782237679f,
    1.682416169f,
    1.588185572f,
    1.499232744f,
    1.415262083f,
    1.335994543f,
    1.261166705f,
    1.190529906f,
    1.123849410f,
    1.060903628f,
    1.001483382f,
    0.945391210f,
    0.892440709f,
    0.842455918f,
    0.795270729f,
    0.750728339f,
    0.708680729f,
    0.668988166f,
    0.631518748f,
    0.596147958f,
    0.562758254f,
    0.531238677f,
    0.501484482f,
    0.473396793f,
    0.446882270f,
    0.421852801f,
    0.398225210f,
    0.375920978f,
    0.354865987f,
    0.334990266f,
    0.316227766f,
    0.298516137f,
    0.281796520f,
    0.266013353f,
    0.251114188f,
    0.237049511f,
    0.223772583f,
    0.211239285f,
    0.199407965f,
    0.188239306f,
    0.177696194f,
    0.167743592f,
    0.158348426f,
    0.149479475f,
    0.141107266f,
    0.133203976f,
    0.125743343f,
    0.118700573f,
    0.112052262f,
    0.105776317f,
    0.099851882f,
    0.094259269f,
    0.088979893f,
    0.083996210f,
    0.079291659f,
    0.074850605f,
    0.070658291f,
    0.066700784f,
    0.062964934f,
    0.059438326f,
    0.056109239f,
    0.052966612f,
    0.050000000f
};

} // namespace

StateVariableFilter::StateVariableFilter()
    : integrator1_(0.0f), integrator2_(0.0f), previousBand_(0.0f) {}

void StateVariableFilter::reset() {
    integrator1_ = 0.0f;
    integrator2_ = 0.0f;
    previousBand_ = 0.0f;
}

float StateVariableFilter::clamp(float value, float low, float high) {
    return value < low ? low : (value > high ? high : value);
}

float StateVariableFilter::finiteClamp(float value, float limit) {
    if (!std::isfinite(value)) {
        return 0.0f;
    }
    return clamp(value, -limit, limit);
}

float StateVariableFilter::softBound(float value, float knee, float limit) {
    if (!std::isfinite(value)) {
        return 0.0f;
    }
    const float magnitude = std::fabs(value);
    if (magnitude <= knee) {
        return value;
    }
    const float distance = magnitude - knee;
    const float bounded = knee
        + distance / (1.0f + distance / (limit - knee));
    return value < 0.0f ? -bounded : bounded;
}

float StateVariableFilter::resonanceDamping(float resonance) {
    const float bounded = clamp(resonance, 0.0f, 1.0f);
    const float tablePosition = bounded
        * static_cast<float>(kResonanceTableSegments);
    const unsigned int index = static_cast<unsigned int>(tablePosition);
    if (index >= kResonanceTableSegments) {
        return kResonanceDampingTable[kResonanceTableSegments];
    }
    const float fraction = tablePosition - static_cast<float>(index);
    return kResonanceDampingTable[index]
        + fraction * (kResonanceDampingTable[index + 1u]
                      - kResonanceDampingTable[index]);
}

StateVariableFilter::Frame StateVariableFilter::process(
    float input, float baseCutoffHz, float cutoffOctaves, float resonance,
    float sampleRate, bool softLimit) {
    const float boundedSampleRate = std::isfinite(sampleRate)
            && sampleRate > 0.0f
        ? sampleRate
        : 48000.0f;
    const float boundedResonance = clamp(resonance, 0.0f, 1.0f);
    const float resonanceSquared = boundedResonance * boundedResonance;
    const float damping = resonanceDamping(boundedResonance);

    const float inputGain = 1.0f
        - kResonanceInputCompensation * resonanceSquared;
    const float conditionedInput = softBound(input * inputGain, 10.0f, 12.0f);

    // R39 is a fixed first-pole-to-frequency-control path. Its character
    // naturally grows with the band-pass amplitude as resonance rises.
    const float skewOctaves = kCharacterSkewOctavesPerVolt * previousBand_;
    const float maximumCutoff = boundedSampleRate * 0.45f;
    const float baseCutoff = clamp(
        std::isfinite(baseCutoffHz) ? baseCutoffHz : 1.0f,
        1.0f, maximumCutoff);
    const float cutoff = clamp(
        baseCutoff * std::pow(2.0f, cutoffOctaves + skewOctaves),
        1.0f, maximumCutoff);

    const float g = std::tan(kPi * cutoff / boundedSampleRate);
    const float a1 = 1.0f / (1.0f + g * (g + damping));
    const float a2 = g * a1;
    const float a3 = g * a2;
    const float v3 = conditionedInput - integrator2_;
    const float bandPass = a1 * integrator1_ + a2 * v3;
    const float lowPass = integrator2_
        + a2 * integrator1_ + a3 * v3;
    const float highPass = conditionedInput
        - damping * bandPass - lowPass;

    integrator1_ = finiteClamp(2.0f * bandPass - integrator1_, 100.0f);
    integrator2_ = finiteClamp(2.0f * lowPass - integrator2_, 100.0f);
    previousBand_ = finiteClamp(bandPass, 100.0f);

    Frame frame;
    frame.lowPass = finiteClamp(lowPass, 100.0f);
    frame.bandPass = finiteClamp(bandPass, 100.0f);
    frame.highPass = finiteClamp(highPass, 100.0f);
    if (softLimit) {
        frame.lowPass = softBound(frame.lowPass, 8.0f, 10.0f);
        frame.bandPass = softBound(frame.bandPass, 8.0f, 10.0f);
        frame.highPass = softBound(frame.highPass, 8.0f, 10.0f);
    }
    return frame;
}

} // namespace burl
