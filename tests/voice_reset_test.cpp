// Copyright (c) 2026 Burl contributors
// SPDX-License-Identifier: MIT

#include "burl/voice.hpp"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdint.h>
#include <string>

namespace {

static_assert(sizeof(float) == sizeof(uint32_t),
              "reset test requires a 32-bit float representation");

int failures = 0;

uint32_t floatBits(float value) {
    uint32_t bits = 0u;
    std::memcpy(&bits, &value, sizeof(bits));
    return bits;
}

void expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

bool outputsEqual(const burl::VoiceOutputs& first,
                  const burl::VoiceOutputs& second) {
    return floatBits(first.lowPass) == floatBits(second.lowPass)
        && floatBits(first.bandPass) == floatBits(second.bandPass)
        && floatBits(first.highPass) == floatBits(second.highPass)
        && floatBits(first.steppedCv) == floatBits(second.steppedCv)
        && floatBits(first.pwm) == floatBits(second.pwm)
        && floatBits(first.registerXor) == floatBits(second.registerXor)
        && floatBits(first.auxA) == floatBits(second.auxA)
        && floatBits(first.auxB) == floatBits(second.auxB);
}

bool statusesEqual(const burl::VoiceStatus& first,
                   const burl::VoiceStatus& second) {
    return floatBits(first.oscillator1Triangle)
            == floatBits(second.oscillator1Triangle)
        && floatBits(first.oscillator2Triangle)
            == floatBits(second.oscillator2Triangle)
        && first.patternState == second.patternState
        && first.active == second.active;
}

float saw(unsigned int frame, unsigned int multiplier, unsigned int period,
          float amplitude) {
    const unsigned int position = (frame * multiplier) % period;
    const float unit = static_cast<float>(position)
        / static_cast<float>(period - 1u);
    return amplitude * (2.0f * unit - 1.0f);
}

burl::VoiceInputs inputsAt(unsigned int frame) {
    burl::VoiceInputs inputs;
    inputs.oscillator1Cv = saw(frame, 7u, 101u, 4.0f);
    inputs.oscillator2Cv = saw(frame, 11u, 137u, 3.5f);
    // Start high so a stale external-clock latch changes the post-reset render.
    inputs.externalClock = ((frame / 13u) & 1u) == 0u ? 5.0f : -5.0f;
    inputs.filterAudio = saw(frame, 13u, 89u, 5.0f);
    inputs.cutoffCv = saw(frame, 5u, 73u, 0.75f);
    return inputs;
}

burl::VoiceParameters targetParameters(burl::QualityMode quality,
                                       bool maximal127Mode,
                                       bool externalClock) {
    burl::VoiceParameters parameters;
    parameters.oscillator1Hz = 127.0f;
    parameters.oscillator2Hz = 193.0f;
    parameters.oscillator1CrossModulation = 0.81f;
    parameters.oscillator2CrossModulation = -0.67f;
    parameters.oscillator1Feedback = 0.43f;
    parameters.oscillator2Feedback = -0.29f;
    parameters.change = 0.57f;
    parameters.filterCutoffHz = 1630.0f;
    parameters.filterResonance = 0.74f;
    parameters.filterFeedback = 0.52f;
    parameters.externalCutoffModulation = 0.31f;
    parameters.externalInputMix = 0.38f;
    parameters.useExternalOscillator1Cv = true;
    parameters.useExternalOscillator2Cv = false;
    parameters.useExternalClock = externalClock;
    parameters.doubleEdgeClock = quality == burl::QualityHigh;
    parameters.maximal127Mode = maximal127Mode;
    parameters.safetyLimit = true;
    parameters.quality = quality;
    parameters.dacMsbTap = 6u;
    parameters.dacMiddleTap = 3u;
    parameters.dacLsbTap = 1u;
    return parameters;
}

void disturbAllState(burl::Voice& voice, float sampleRate,
                     const burl::VoiceParameters& target) {
    burl::VoiceParameters mutation = target;
    mutation.quality = burl::QualityEco;
    mutation.maximal127Mode = !target.maximal127Mode;
    mutation.oscillator1Hz = sampleRate / 23.0f;
    mutation.oscillator2Hz = sampleRate / 16.0f;
    voice.setParameters(mutation);
    voice.reset();

    burl::VoiceInputs inputs;
    if (target.useExternalClock) {
        mutation.useExternalClock = true;
        voice.setParameters(mutation);
        inputs.externalClock = -5.0f;
        voice.process(inputs);
        inputs.externalClock = 5.0f;
        voice.process(inputs); // Leave the Schmitt latch high.
    } else {
        mutation.useExternalClock = false;
        voice.setParameters(mutation);
        for (unsigned int frame = 0u; frame < 6u; ++frame) {
            voice.process(inputs);
        } // Leave oscillator 2 moving upward and its pulse detector high.
    }
}

uint8_t expectedResetPattern(uint8_t seed, bool maximal127Mode) {
    if (!maximal127Mode) {
        return seed;
    }
    const uint8_t sevenBit = static_cast<uint8_t>(seed & 0x7fu);
    return sevenBit == 0u ? 1u : sevenBit;
}

const char* qualityName(burl::QualityMode quality) {
    switch (quality) {
    case burl::QualityEco:
        return "Eco";
    case burl::QualityNormal:
        return "Normal";
    case burl::QualityHigh:
        return "High";
    }
    return "unknown";
}

std::string contextName(unsigned int sampleRate, burl::QualityMode quality,
                        bool maximal127Mode, bool externalClock, uint8_t seed) {
    std::string name = std::to_string(sampleRate) + " Hz, ";
    name += qualityName(quality);
    name += maximal127Mode ? ", 127 mode" : ", 8/16 mode";
    name += externalClock ? ", external clock" : ", internal clock";
    name += seed == 0u ? ", zero seed" : ", non-zero seed";
    return name;
}

void testResetRestoresCompleteKnownState() {
    const unsigned int sampleRates[] = {32000u, 44100u, 48000u, 88200u, 96000u};
    const burl::QualityMode qualities[] = {
        burl::QualityEco, burl::QualityNormal, burl::QualityHigh
    };
    const uint8_t seeds[] = {0u, 0xa5u};

    for (unsigned int rateIndex = 0u; rateIndex < 5u; ++rateIndex) {
        for (unsigned int qualityIndex = 0u; qualityIndex < 3u; ++qualityIndex) {
            for (unsigned int mode = 0u; mode < 2u; ++mode) {
                for (unsigned int clock = 0u; clock < 2u; ++clock) {
                    for (unsigned int seedIndex = 0u; seedIndex < 2u; ++seedIndex) {
                        const unsigned int sampleRate = sampleRates[rateIndex];
                        const burl::QualityMode quality = qualities[qualityIndex];
                        const bool maximal127Mode = mode != 0u;
                        const bool externalClock = clock != 0u;
                        const uint8_t seed = seeds[seedIndex];
                        const std::string context = contextName(
                            sampleRate, quality, maximal127Mode, externalClock,
                            seed);
                        const burl::VoiceParameters parameters = targetParameters(
                            quality, maximal127Mode, externalClock);

                        burl::Voice reference(static_cast<float>(sampleRate), seed);
                        reference.setParameters(parameters);
                        reference.reset();

                        burl::Voice resetVoice(static_cast<float>(sampleRate), seed);
                        disturbAllState(resetVoice, static_cast<float>(sampleRate),
                                        parameters);
                        expect(resetVoice.status().active,
                               context + ": mutation must activate the voice");
                        resetVoice.setSeed(seed);
                        resetVoice.setParameters(parameters);
                        resetVoice.reset();

                        const burl::VoiceStatus resetStatus = resetVoice.status();
                        expect(resetVoice.seed() == seed,
                               context + ": reset must retain the configured seed");
                        expect(floatBits(resetStatus.oscillator1Triangle)
                                   == floatBits(-2.5f),
                               context + ": oscillator 1 must return to -2.5 V");
                        expect(floatBits(resetStatus.oscillator2Triangle)
                                   == floatBits(1.25f),
                               context + ": oscillator 2 must return to +1.25 V");
                        expect(resetStatus.patternState
                                   == expectedResetPattern(seed, maximal127Mode),
                               context + ": pattern state must return to its configured reset value");
                        expect(!resetStatus.active,
                               context + ": reset status must be inactive until processing resumes");
                        expect(statusesEqual(resetStatus, reference.status()),
                               context + ": reset status must match a fresh configured voice");

                        for (unsigned int frame = 0u; frame < 512u; ++frame) {
                            const burl::VoiceInputs inputs = inputsAt(frame);
                            const burl::VoiceOutputs referenceOutput =
                                reference.process(inputs);
                            const burl::VoiceOutputs resetOutput =
                                resetVoice.process(inputs);
                            if (!outputsEqual(referenceOutput, resetOutput)
                                || !statusesEqual(reference.status(),
                                                  resetVoice.status())) {
                                expect(false, context
                                           + ": post-reset render diverged from fresh voice at frame "
                                           + std::to_string(frame));
                                break;
                            }
                            if (maximal127Mode) {
                                expect(resetVoice.status().patternState != 0u,
                                       context
                                           + ": 127 mode entered the zero lock state at frame "
                                           + std::to_string(frame));
                            }
                        }
                    }
                }
            }
        }
    }
}

} // namespace

int main() {
    testResetRestoresCompleteKnownState();

    if (failures != 0) {
        std::cerr << failures << " voice reset assertion(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "All voice reset tests passed (120 contexts, 61440 post-reset frames)\n";
    return EXIT_SUCCESS;
}
