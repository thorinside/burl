// Copyright (c) 2026 Burl contributors
// SPDX-License-Identifier: MIT

#include "burl/voice.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <stdint.h>

namespace {

const unsigned int kFramesPerContext = 32768u;
const unsigned int kFramesPerPhase = 4096u;
const float kOutputLimit = 10.0f;

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

float endpointSequence(unsigned int frame, unsigned int stride,
                       unsigned int offset, float magnitude) {
    const int values[] = {-4, 4, -3, 3, -2, 2, -1, 1, 0};
    const unsigned int count = sizeof(values) / sizeof(values[0]);
    const unsigned int index = ((frame / stride) + offset) % count;
    return magnitude * static_cast<float>(values[index]) / 4.0f;
}

burl::VoiceInputs inputsAt(unsigned int frame) {
    burl::VoiceInputs inputs;
    inputs.oscillator1Cv = endpointSequence(frame, 13u, 0u, 10.0f);
    inputs.oscillator2Cv = endpointSequence(frame, 17u, 3u, 10.0f);
    inputs.filterAudio = endpointSequence(frame, 11u, 5u, 12.0f);
    inputs.cutoffCv = endpointSequence(frame, 19u, 7u, 10.0f);

    const unsigned int clockPhase = (frame / 7u) % 4u;
    if (clockPhase == 0u) {
        inputs.externalClock = -5.0f;
    } else if (clockPhase == 2u) {
        inputs.externalClock = 5.0f;
    } else {
        inputs.externalClock = 0.0f;
    }
    return inputs;
}

burl::VoiceParameters parametersFor(float sampleRate,
                                    burl::QualityMode quality,
                                    unsigned int phase) {
    const float oscillator1Hz[] = {
        0.001f, 1.0f, 20.0f, 440.0f,
        sampleRate * 0.45f, sampleRate * 4.0f, 20000.0f, 73.0f
    };
    const float oscillator2Hz[] = {
        sampleRate * 4.0f, 20000.0f, sampleRate * 0.45f, 31.0f,
        0.001f, 880.0f, 2.0f, 997.0f
    };
    const float modulation[] = {
        -16.0f, 16.0f, -8.0f, 8.0f, -1.0f, 1.0f, 0.0f, 4.0f
    };
    const float cutoffHz[] = {
        1.0f, sampleRate * 0.45f, sampleRate * 4.0f, 20.0f,
        20000.0f, 400.0f, 1200.0f, 73.0f
    };
    const float change[] = {0.0f, 1.0f, 0.5f, 0.01f, 0.99f, 0.25f, 0.75f, 0.42f};
    const unsigned int boundedPhase = phase & 7u;

    burl::VoiceParameters parameters;
    parameters.oscillator1Hz = oscillator1Hz[boundedPhase];
    parameters.oscillator2Hz = oscillator2Hz[boundedPhase];
    parameters.oscillator1CrossModulation = modulation[boundedPhase];
    parameters.oscillator2CrossModulation = modulation[7u - boundedPhase];
    parameters.oscillator1Feedback = modulation[(boundedPhase + 2u) & 7u];
    parameters.oscillator2Feedback = modulation[(boundedPhase + 5u) & 7u];
    parameters.change = change[boundedPhase];
    parameters.filterCutoffHz = cutoffHz[boundedPhase];
    parameters.filterResonance = (boundedPhase & 1u) != 0u ? 1.0f : 0.0f;
    parameters.filterFeedback = modulation[(boundedPhase + 1u) & 7u];
    parameters.externalCutoffModulation = modulation[(boundedPhase + 4u) & 7u];
    parameters.externalInputMix = static_cast<float>(boundedPhase) / 7.0f;
    parameters.useExternalOscillator1Cv = (boundedPhase & 1u) != 0u;
    parameters.useExternalOscillator2Cv = (boundedPhase & 2u) != 0u;
    parameters.useExternalClock = (boundedPhase & 4u) != 0u;
    parameters.doubleEdgeClock = (boundedPhase & 2u) != 0u;
    parameters.maximal127Mode = (boundedPhase & 1u) != 0u;
    parameters.safetyLimit = true;
    parameters.quality = quality;
    parameters.dacMsbTap = boundedPhase;
    parameters.dacMiddleTap = (boundedPhase + 3u) & 7u;
    parameters.dacLsbTap = (boundedPhase + 6u) & 7u;
    return parameters;
}

bool validateFrame(const burl::VoiceOutputs& outputs,
                   const burl::VoiceStatus& status, unsigned int sampleRate,
                   burl::QualityMode quality, uint8_t seed,
                   unsigned int frame) {
    const float values[] = {
        outputs.lowPass,
        outputs.bandPass,
        outputs.highPass,
        outputs.steppedCv,
        outputs.pwm,
        outputs.registerXor,
        outputs.auxA,
        outputs.auxB
    };
    const char* names[] = {
        "LP", "BP", "HP", "stepped CV", "PWM", "register XOR", "Aux A", "Aux B"
    };

    for (unsigned int output = 0u; output < 8u; ++output) {
        if (!std::isfinite(values[output])
            || std::fabs(values[output]) > kOutputLimit) {
            std::cerr << "FAIL: " << names[output] << " output "
                      << values[output] << " escaped the +/-" << kOutputLimit
                      << " V safety bound at frame " << frame << ", "
                      << sampleRate << " Hz, " << qualityName(quality)
                      << " quality, seed " << static_cast<unsigned int>(seed)
                      << '\n';
            return false;
        }
    }

    if (!std::isfinite(status.oscillator1Triangle)
        || !std::isfinite(status.oscillator2Triangle)
        || std::fabs(status.oscillator1Triangle) > 5.0f
        || std::fabs(status.oscillator2Triangle) > 5.0f) {
        std::cerr << "FAIL: oscillator state became non-finite or unbounded at frame "
                  << frame << ", " << sampleRate << " Hz, "
                  << qualityName(quality) << " quality, seed "
                  << static_cast<unsigned int>(seed) << '\n';
        return false;
    }

    if (!status.active) {
        std::cerr << "FAIL: voice did not report active after processing at frame "
                  << frame << '\n';
        return false;
    }
    return true;
}

bool runContext(unsigned int sampleRate, burl::QualityMode quality,
                uint8_t seed) {
    burl::Voice voice(static_cast<float>(sampleRate), seed);
    unsigned int activePhase = 0u;
    voice.setParameters(parametersFor(static_cast<float>(sampleRate), quality,
                                      activePhase));
    voice.reset();

    for (unsigned int frame = 0u; frame < kFramesPerContext; ++frame) {
        const unsigned int phase = frame / kFramesPerPhase;
        if (phase != activePhase) {
            activePhase = phase;
            voice.setParameters(parametersFor(static_cast<float>(sampleRate),
                                              quality, activePhase));
        }

        const burl::VoiceOutputs outputs = voice.process(inputsAt(frame));
        if (!validateFrame(outputs, voice.status(), sampleRate, quality, seed,
                           frame)) {
            return false;
        }
    }
    return true;
}

int stressAllSupportedRatesAndQualities() {
    const unsigned int sampleRates[] = {32000u, 44100u, 48000u, 88200u, 96000u};
    const burl::QualityMode qualities[] = {
        burl::QualityEco, burl::QualityNormal, burl::QualityHigh
    };
    const uint8_t seeds[] = {0x01u, 0xa5u};

    int failures = 0;
    for (unsigned int rate = 0u; rate < 5u; ++rate) {
        for (unsigned int quality = 0u; quality < 3u; ++quality) {
            for (unsigned int seed = 0u; seed < 2u; ++seed) {
                if (!runContext(sampleRates[rate], qualities[quality],
                                seeds[seed])) {
                    ++failures;
                }
            }
        }
    }
    return failures;
}

struct FloatParameterCase {
    const char* name;
    float burl::VoiceParameters::*member;
};

bool validateNonFiniteParameter(const FloatParameterCase& parameterCase,
                                float invalidValue,
                                burl::QualityMode quality,
                                unsigned int caseIndex) {
    const unsigned int sampleRate = 48000u;
    burl::Voice voice(static_cast<float>(sampleRate), 0x5du);
    const burl::VoiceParameters valid = parametersFor(
        static_cast<float>(sampleRate), quality, 7u);
    voice.setParameters(valid);
    voice.reset();
    voice.process(inputsAt(caseIndex));

    burl::VoiceParameters invalid = valid;
    invalid.*(parameterCase.member) = invalidValue;
    voice.setParameters(invalid);
    const burl::VoiceOutputs invalidOutputs = voice.process(
        inputsAt(caseIndex + 1u));
    const burl::VoiceStatus invalidStatus = voice.status();
    if (!validateFrame(invalidOutputs, invalidStatus, sampleRate, quality,
                       0x5du, caseIndex + 1u)) {
        std::cerr << "FAIL: non-finite " << parameterCase.name
                  << " escaped voice conditioning\n";
        return false;
    }

    voice.setParameters(valid);
    const burl::VoiceOutputs recoveredOutputs = voice.process(
        inputsAt(caseIndex + 2u));
    const burl::VoiceStatus recoveredStatus = voice.status();
    if (!validateFrame(recoveredOutputs, recoveredStatus, sampleRate, quality,
                       0x5du, caseIndex + 2u)) {
        std::cerr << "FAIL: voice did not recover after non-finite "
                  << parameterCase.name << '\n';
        return false;
    }
    if (recoveredStatus.oscillator1Triangle
            == invalidStatus.oscillator1Triangle
        && recoveredStatus.oscillator2Triangle
            == invalidStatus.oscillator2Triangle) {
        std::cerr << "FAIL: oscillators did not resume after non-finite "
                  << parameterCase.name << '\n';
        return false;
    }
    return true;
}

int stressNonFiniteParameters() {
    const FloatParameterCase parameters[] = {
        {"oscillator1Hz", &burl::VoiceParameters::oscillator1Hz},
        {"oscillator2Hz", &burl::VoiceParameters::oscillator2Hz},
        {"oscillator1CrossModulation",
         &burl::VoiceParameters::oscillator1CrossModulation},
        {"oscillator2CrossModulation",
         &burl::VoiceParameters::oscillator2CrossModulation},
        {"oscillator1Feedback", &burl::VoiceParameters::oscillator1Feedback},
        {"oscillator2Feedback", &burl::VoiceParameters::oscillator2Feedback},
        {"change", &burl::VoiceParameters::change},
        {"filterCutoffHz", &burl::VoiceParameters::filterCutoffHz},
        {"filterResonance", &burl::VoiceParameters::filterResonance},
        {"filterFeedback", &burl::VoiceParameters::filterFeedback},
        {"externalCutoffModulation",
         &burl::VoiceParameters::externalCutoffModulation},
        {"externalInputMix", &burl::VoiceParameters::externalInputMix},
        {"changeCvAmount", &burl::VoiceParameters::changeCvAmount},
        {"resonanceCvAmount", &burl::VoiceParameters::resonanceCvAmount},
        {"mixCvAmount", &burl::VoiceParameters::mixCvAmount},
        {"inputDrive", &burl::VoiceParameters::inputDrive}
    };
    const float invalidValues[] = {
        std::numeric_limits<float>::quiet_NaN(),
        std::numeric_limits<float>::infinity(),
        -std::numeric_limits<float>::infinity()
    };
    const burl::QualityMode qualities[] = {
        burl::QualityEco, burl::QualityNormal, burl::QualityHigh
    };

    int failures = 0;
    unsigned int caseIndex = 0u;
    for (unsigned int quality = 0u; quality < 3u; ++quality) {
        for (unsigned int parameter = 0u;
             parameter < sizeof(parameters) / sizeof(parameters[0]);
             ++parameter) {
            for (unsigned int value = 0u;
                 value < sizeof(invalidValues) / sizeof(invalidValues[0]);
                 ++value) {
                if (!validateNonFiniteParameter(parameters[parameter],
                                                invalidValues[value],
                                                qualities[quality], caseIndex)) {
                    ++failures;
                }
                caseIndex += 3u;
            }
        }
    }
    return failures;
}

} // namespace

int main() {
    const int failures = stressAllSupportedRatesAndQualities()
        + stressNonFiniteParameters();
    if (failures != 0) {
        std::cerr << failures << " voice stress context(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "All voice stress tests passed (30 finite contexts plus 144 "
                 "non-finite parameter cases)\n";
    return EXIT_SUCCESS;
}
