// Copyright (c) 2026 Burl contributors
// SPDX-License-Identifier: MIT

#include "burl/voice.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <stdint.h>

namespace {

int failures = 0;

void expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

void expectNear(float actual, float expected, float tolerance,
                const char* message) {
    if (!std::isfinite(actual)
        || std::fabs(actual - expected) > tolerance) {
        std::cerr << "FAIL: " << message << " (expected " << expected
                  << ", got " << actual << ")\n";
        ++failures;
    }
}

burl::VoiceParameters oscillatorParameters() {
    burl::VoiceParameters parameters;
    parameters.oscillator1Hz = 16.0f;
    parameters.oscillator2Hz = 16.0f;
    parameters.oscillator1CrossModulation = 1.0f;
    parameters.oscillator2CrossModulation = 1.0f;
    parameters.oscillator1Feedback = 0.0f;
    parameters.oscillator2Feedback = 0.0f;
    parameters.quality = burl::QualityEco;
    return parameters;
}

burl::VoiceStatus processOnce(const burl::VoiceParameters& parameters,
                              const burl::VoiceInputs& inputs) {
    burl::Voice voice(1024.0f, 1u);
    voice.setParameters(parameters);
    voice.reset();
    voice.process(inputs);
    return voice.status();
}

void testOscillatorCvRoutesReplaceInternalNormals() {
    burl::VoiceParameters normalParameters = oscillatorParameters();

    burl::VoiceInputs negativeInputs;
    negativeInputs.oscillator1Cv = -10.0f;
    negativeInputs.oscillator2Cv = -10.0f;
    burl::VoiceInputs positiveInputs;
    positiveInputs.oscillator1Cv = 10.0f;
    positiveInputs.oscillator2Cv = 10.0f;

    const burl::VoiceStatus normalWithNegativeInput =
        processOnce(normalParameters, negativeInputs);
    const burl::VoiceStatus normalWithPositiveInput =
        processOnce(normalParameters, positiveInputs);
    expectNear(normalWithNegativeInput.oscillator1Triangle,
               normalWithPositiveInput.oscillator1Triangle, 0.0f,
               "OSC1 must ignore its external CV when the internal normal is selected");
    expectNear(normalWithNegativeInput.oscillator2Triangle,
               normalWithPositiveInput.oscillator2Triangle, 0.0f,
               "OSC2 must ignore its external CV when the internal normal is selected");

    burl::VoiceInputs zeroInputs;

    burl::VoiceParameters externalOscillator1 = normalParameters;
    externalOscillator1.useExternalOscillator1Cv = true;
    const burl::VoiceStatus oscillator1ExternalZero =
        processOnce(externalOscillator1, zeroInputs);
    burl::VoiceParameters oscillator1NoModulation = normalParameters;
    oscillator1NoModulation.oscillator1CrossModulation = 0.0f;
    const burl::VoiceStatus oscillator1Control =
        processOnce(oscillator1NoModulation, zeroInputs);
    expectNear(oscillator1ExternalZero.oscillator1Triangle,
               oscillator1Control.oscillator1Triangle, 0.0f,
               "zero external OSC1 CV must replace, not add to, the OSC2 triangle normal");
    expect(oscillator1ExternalZero.oscillator1Triangle
               != normalWithNegativeInput.oscillator1Triangle,
           "selecting external OSC1 CV must remove the non-zero internal normal");
    expectNear(oscillator1ExternalZero.oscillator1Triangle, -2.1875f, 0.0f,
               "external zero OSC1 CV must leave only the baseline frequency");

    burl::VoiceParameters externalOscillator2 = normalParameters;
    externalOscillator2.useExternalOscillator2Cv = true;
    const burl::VoiceStatus oscillator2ExternalZero =
        processOnce(externalOscillator2, zeroInputs);
    burl::VoiceParameters oscillator2NoModulation = normalParameters;
    oscillator2NoModulation.oscillator2CrossModulation = 0.0f;
    const burl::VoiceStatus oscillator2Control =
        processOnce(oscillator2NoModulation, zeroInputs);
    expectNear(oscillator2ExternalZero.oscillator2Triangle,
               oscillator2Control.oscillator2Triangle, 0.0f,
               "zero external OSC2 CV must replace, not add to, the OSC1 triangle normal");
    expect(oscillator2ExternalZero.oscillator2Triangle
               != normalWithNegativeInput.oscillator2Triangle,
           "selecting external OSC2 CV must remove the non-zero internal normal");
    expectNear(oscillator2ExternalZero.oscillator2Triangle, 0.9375f, 0.0f,
               "external zero OSC2 CV must leave only the baseline frequency");
}

burl::VoiceParameters clockParameters(bool useExternalClock) {
    burl::VoiceParameters parameters;
    parameters.oscillator1Hz = 16.0f;
    parameters.oscillator2Hz = 256.0f;
    parameters.oscillator1CrossModulation = 0.0f;
    parameters.oscillator2CrossModulation = 0.0f;
    parameters.oscillator1Feedback = 0.0f;
    parameters.oscillator2Feedback = 0.0f;
    parameters.change = 0.0f;
    parameters.useExternalClock = useExternalClock;
    parameters.quality = burl::QualityEco;
    return parameters;
}

void testClockRouteReplacesInternalClock() {
    const uint8_t seed = 0x01u;
    burl::VoiceInputs highClock;
    highClock.externalClock = 5.0f;
    burl::VoiceInputs lowClock;
    lowClock.externalClock = -5.0f;

    burl::Voice internalClock(1024.0f, seed);
    internalClock.setParameters(clockParameters(false));
    internalClock.reset();
    internalClock.process(highClock);
    expect(internalClock.status().patternState == seed,
           "external clock edge must be ignored when the internal clock is selected");
    internalClock.process(lowClock);
    expect(internalClock.status().patternState == 0x80u,
           "internal oscillator-2 rising edge must clock when its normal is selected");

    burl::Voice externalClockHeldLow(1024.0f, seed);
    externalClockHeldLow.setParameters(clockParameters(true));
    externalClockHeldLow.reset();
    externalClockHeldLow.process(lowClock);
    externalClockHeldLow.process(lowClock);
    expect(externalClockHeldLow.status().patternState == seed,
           "internal oscillator-2 edge must be ignored when external clock is selected");

    burl::Voice externalClockDriven(1024.0f, seed);
    externalClockDriven.setParameters(clockParameters(true));
    externalClockDriven.reset();
    externalClockDriven.process(highClock);
    expect(externalClockDriven.status().patternState == 0x80u,
           "external Schmitt rising edge must clock when external clock is selected");
}

} // namespace

int main() {
    testOscillatorCvRoutesReplaceInternalNormals();
    testClockRouteReplacesInternalClock();

    if (failures != 0) {
        std::cerr << failures << " source-routing assertion(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "All source-routing tests passed\n";
    return EXIT_SUCCESS;
}
