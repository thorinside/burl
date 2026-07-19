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

burl::VoiceParameters filterSourceParameters(bool externalInput) {
    burl::VoiceParameters parameters;
    parameters.oscillator1Hz = 41.0f;
    parameters.oscillator2Hz = 73.0f;
    parameters.oscillator1CrossModulation = 0.37f;
    parameters.oscillator2CrossModulation = -0.21f;
    parameters.oscillator1Feedback = 0.18f;
    parameters.oscillator2Feedback = -0.12f;
    parameters.change = 0.43f;
    parameters.filterCutoffHz = 617.0f;
    parameters.filterResonance = 0.74f;
    parameters.filterFeedback = 0.0f;
    parameters.externalCutoffModulation = 0.0f;
    parameters.externalInputMix = externalInput ? 1.0f : 0.0f;
    parameters.inputDrive = 1.0f;
    parameters.safetyLimit = false;
    parameters.quality = burl::QualityEco;
    return parameters;
}

void testInternalPwmAndSteppedCvSourceMatchesExternalReference() {
    const uint8_t seed = 0x5du;
    burl::Voice internalSource(48000.0f, seed);
    burl::Voice externalReference(48000.0f, seed);
    internalSource.setParameters(filterSourceParameters(false));
    externalReference.setParameters(filterSourceParameters(true));
    internalSource.reset();
    externalReference.reset();

    burl::VoiceInputs silentInputs;
    for (unsigned int frame = 0u; frame < 8192u; ++frame) {
        const burl::VoiceOutputs sourceOutput = internalSource.process(
            silentInputs);
        burl::VoiceInputs referenceInputs;
        // External audio is a nominal +/-5 V source. Normalize the published
        // V1 PWM/RUNCV summer to that range so both sides of Input mix have
        // the same filter-summer headroom.
        referenceInputs.filterAudio =
            (0.021872f * sourceOutput.pwm
             + 0.044231f * sourceOutput.steppedCv)
            / 0.066103f;
        const burl::VoiceOutputs referenceOutput = externalReference.process(
            referenceInputs);

        expectNear(sourceOutput.lowPass, referenceOutput.lowPass, 0.000001f,
                   "internal LP source must match the V1 input-network forcing");
        expectNear(sourceOutput.bandPass, referenceOutput.bandPass, 0.000001f,
                   "internal BP source must match the V1 input-network forcing");
        expectNear(sourceOutput.highPass, referenceOutput.highPass, 0.000001f,
                   "internal HP source must match the V1 input-network forcing");
        if (failures != 0) {
            break;
        }
    }
}

void testNominalExternalAudioDoesNotOverdriveV1Summer() {
    burl::VoiceParameters parameters = filterSourceParameters(true);
    parameters.filterCutoffHz = 250.0f;
    parameters.filterResonance = 0.62f;
    parameters.inputDrive = 1.0f;
    parameters.safetyLimit = true;
    parameters.quality = burl::QualityNormal;

    burl::Voice voice(48000.0f, 0x5du);
    voice.setParameters(parameters);
    voice.reset();

    const unsigned int warmupFrames = 24000u;
    const unsigned int measurementFrames = 480000u;
    unsigned int limitedFrames = 0u;
    for (unsigned int frame = 0u;
         frame < warmupFrames + measurementFrames; ++frame) {
        burl::VoiceInputs inputs;
        const float phase = std::fmod(
            53.0f * static_cast<float>(frame) / 48000.0f, 1.0f);
        inputs.filterAudio = phase < 0.5f ? 5.0f : -5.0f;
        const burl::VoiceOutputs output = voice.process(inputs);
        if (frame >= warmupFrames
            && (std::fabs(output.lowPass) >= 8.0f
                || std::fabs(output.bandPass) >= 8.0f
                || std::fabs(output.highPass) >= 8.0f)) {
            ++limitedFrames;
        }
    }

    expect(limitedFrames <= measurementFrames / 100u,
           "nominal +/-5 V external audio at 1x must retain V1 summer headroom");
}

} // namespace

int main() {
    testOscillatorCvRoutesReplaceInternalNormals();
    testClockRouteReplacesInternalClock();
    testInternalPwmAndSteppedCvSourceMatchesExternalReference();
    testNominalExternalAudioDoesNotOverdriveV1Summer();

    if (failures != 0) {
        std::cerr << failures << " source-routing assertion(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "All source-routing tests passed\n";
    return EXIT_SUCCESS;
}
