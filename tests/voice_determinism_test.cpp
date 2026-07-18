// Copyright (c) 2026 Burl contributors
// SPDX-License-Identifier: MIT

#include "burl/voice.hpp"

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdint.h>
#include <vector>

namespace {

static_assert(sizeof(float) == sizeof(uint32_t),
              "determinism test requires a 32-bit float representation");

struct RenderContext {
    unsigned int sampleRate;
    burl::QualityMode quality;
    bool externalRoutes;
    uint8_t seed;
};

struct RenderFrame {
    uint32_t outputs[8];
    uint32_t oscillator1;
    uint32_t oscillator2;
    uint8_t patternState;
    bool active;
};

struct RenderResult {
    RenderFrame initialState;
    std::vector<RenderFrame> frames;
};

uint32_t floatBits(float value) {
    uint32_t bits = 0u;
    std::memcpy(&bits, &value, sizeof(bits));
    return bits;
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

float saw(unsigned int index, unsigned int multiplier, unsigned int period,
          float amplitude) {
    const unsigned int position = (index * multiplier) % period;
    const float unit = static_cast<float>(position)
        / static_cast<float>(period - 1u);
    return amplitude * (2.0f * unit - 1.0f);
}

burl::VoiceInputs inputsAt(unsigned int frame) {
    burl::VoiceInputs inputs;
    inputs.oscillator1Cv = saw(frame, 7u, 101u, 4.0f);
    inputs.oscillator2Cv = saw(frame, 11u, 137u, 3.5f);
    inputs.externalClock = ((frame / 17u) & 1u) != 0u ? 5.0f : -5.0f;
    inputs.filterAudio = saw(frame, 13u, 89u, 5.0f);
    inputs.cutoffCv = saw(frame, 5u, 73u, 0.75f);
    return inputs;
}

burl::VoiceParameters parametersFor(const RenderContext& context) {
    burl::VoiceParameters parameters;
    parameters.oscillator1Hz = 83.0f;
    parameters.oscillator2Hz = 211.0f;
    parameters.oscillator1CrossModulation = 0.87f;
    parameters.oscillator2CrossModulation = -0.63f;
    parameters.oscillator1Feedback = 0.42f;
    parameters.oscillator2Feedback = -0.31f;
    parameters.change = 0.46f;
    parameters.filterCutoffHz = 1470.0f;
    parameters.filterResonance = 0.72f;
    parameters.filterFeedback = 0.58f;
    parameters.externalCutoffModulation = 0.37f;
    parameters.externalInputMix = 0.35f;
    parameters.useExternalOscillator1Cv = context.externalRoutes;
    parameters.useExternalOscillator2Cv = !context.externalRoutes;
    parameters.useExternalClock = context.externalRoutes;
    parameters.doubleEdgeClock = context.quality == burl::QualityHigh;
    parameters.maximal127Mode = !context.externalRoutes;
    parameters.safetyLimit = true;
    parameters.quality = context.quality;
    parameters.dacMsbTap = 7u;
    parameters.dacMiddleTap = 4u;
    parameters.dacLsbTap = 1u;
    return parameters;
}

RenderFrame capture(const burl::VoiceOutputs& outputs,
                    const burl::VoiceStatus& status) {
    RenderFrame frame;
    frame.outputs[0] = floatBits(outputs.lowPass);
    frame.outputs[1] = floatBits(outputs.bandPass);
    frame.outputs[2] = floatBits(outputs.highPass);
    frame.outputs[3] = floatBits(outputs.steppedCv);
    frame.outputs[4] = floatBits(outputs.pwm);
    frame.outputs[5] = floatBits(outputs.registerXor);
    frame.outputs[6] = floatBits(outputs.auxA);
    frame.outputs[7] = floatBits(outputs.auxB);
    frame.oscillator1 = floatBits(status.oscillator1Triangle);
    frame.oscillator2 = floatBits(status.oscillator2Triangle);
    frame.patternState = status.patternState;
    frame.active = status.active;
    return frame;
}

RenderResult render(const RenderContext& context) {
    burl::Voice voice(static_cast<float>(context.sampleRate), context.seed);
    burl::VoiceParameters parameters = parametersFor(context);
    voice.setParameters(parameters);
    voice.reset();

    burl::VoiceOutputs lastOutput;
    for (unsigned int frame = 0u; frame < 257u; ++frame) {
        lastOutput = voice.process(inputsAt(frame));
    }

    RenderResult result;
    result.initialState = capture(lastOutput, voice.status());
    result.frames.reserve(2048u);

    for (unsigned int frame = 0u; frame < 2048u; ++frame) {
        if (frame == 1024u) {
            parameters.change = 0.73f;
            parameters.filterResonance = 0.88f;
            parameters.externalInputMix = 0.61f;
            voice.setParameters(parameters);
        }
        const burl::VoiceOutputs outputs = voice.process(inputsAt(frame + 257u));
        result.frames.push_back(capture(outputs, voice.status()));
    }

    return result;
}

bool framesEqual(const RenderFrame& first, const RenderFrame& second) {
    for (unsigned int output = 0u; output < 8u; ++output) {
        if (first.outputs[output] != second.outputs[output]) {
            return false;
        }
    }
    return first.oscillator1 == second.oscillator1
        && first.oscillator2 == second.oscillator2
        && first.patternState == second.patternState
        && first.active == second.active;
}

bool frameIsFinite(const RenderFrame& frame) {
    for (unsigned int output = 0u; output < 8u; ++output) {
        float value = 0.0f;
        std::memcpy(&value, &frame.outputs[output], sizeof(value));
        if (!std::isfinite(value)) {
            return false;
        }
    }
    return true;
}

int testRepeatedVoiceRendersAreBitExact() {
    const unsigned int sampleRates[] = {32000u, 44100u, 48000u, 88200u, 96000u};
    const burl::QualityMode qualities[] = {
        burl::QualityEco, burl::QualityNormal, burl::QualityHigh
    };

    int failures = 0;
    for (unsigned int rateIndex = 0u; rateIndex < 5u; ++rateIndex) {
        for (unsigned int qualityIndex = 0u; qualityIndex < 3u; ++qualityIndex) {
            for (unsigned int routes = 0u; routes < 2u; ++routes) {
                RenderContext context;
                context.sampleRate = sampleRates[rateIndex];
                context.quality = qualities[qualityIndex];
                context.externalRoutes = routes != 0u;
                context.seed = static_cast<uint8_t>(
                    0x29u + rateIndex * 19u + qualityIndex * 23u + routes * 41u);

                const RenderResult first = render(context);
                const RenderResult second = render(context);
                if (!framesEqual(first.initialState, second.initialState)) {
                    std::cerr << "FAIL: initial voice state differs at "
                              << context.sampleRate << " Hz, "
                              << qualityName(context.quality) << " quality, "
                              << (context.externalRoutes ? "external" : "normal")
                              << " routes\n";
                    ++failures;
                    continue;
                }

                if (first.frames.size() != second.frames.size()
                    || first.frames.empty()) {
                    std::cerr << "FAIL: voice render length differs or is empty\n";
                    ++failures;
                    continue;
                }

                bool changed = false;
                for (std::size_t frame = 0u; frame < first.frames.size(); ++frame) {
                    if (!framesEqual(first.frames[frame], second.frames[frame])) {
                        std::cerr << "FAIL: voice render differs at frame " << frame
                                  << ", " << context.sampleRate << " Hz, "
                                  << qualityName(context.quality) << " quality\n";
                        ++failures;
                        break;
                    }
                    if (!frameIsFinite(first.frames[frame])) {
                        std::cerr << "FAIL: non-finite voice output at frame "
                                  << frame << '\n';
                        ++failures;
                        break;
                    }
                    if (frame != 0u
                        && !framesEqual(first.frames[0], first.frames[frame])) {
                        changed = true;
                    }
                }

                if (!changed) {
                    std::cerr << "FAIL: deterministic voice render is vacuously constant\n";
                    ++failures;
                }
            }
        }
    }

    return failures;
}

} // namespace

int main() {
    const int failures = testRepeatedVoiceRendersAreBitExact();
    if (failures != 0) {
        std::cerr << failures << " deterministic voice-render check(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "All deterministic voice-render tests passed\n";
    return EXIT_SUCCESS;
}
