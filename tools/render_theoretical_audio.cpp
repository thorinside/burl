// Copyright (c) 2026 Burl contributors
// SPDX-License-Identifier: MIT

#include "burl/voice.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdint.h>
#include <string>
#include <vector>

namespace {

const unsigned int kSampleRate = 48000u;
const unsigned int kRenderSeconds = 6u;

void writeUint16(std::ofstream& stream, uint16_t value) {
    const char bytes[] = {
        static_cast<char>(value & 0xffu),
        static_cast<char>((value >> 8u) & 0xffu)
    };
    stream.write(bytes, sizeof(bytes));
}

void writeUint32(std::ofstream& stream, uint32_t value) {
    const char bytes[] = {
        static_cast<char>(value & 0xffu),
        static_cast<char>((value >> 8u) & 0xffu),
        static_cast<char>((value >> 16u) & 0xffu),
        static_cast<char>((value >> 24u) & 0xffu)
    };
    stream.write(bytes, sizeof(bytes));
}

bool writeMonoWav(const std::string& path,
                  const std::vector<float>& voltages) {
    std::ofstream stream(path.c_str(), std::ios::out | std::ios::binary);
    if (!stream) {
        std::cerr << "could not create " << path << '\n';
        return false;
    }

    const uint16_t channels = 1u;
    const uint16_t bitsPerSample = 16u;
    const uint32_t bytesPerSample = bitsPerSample / 8u;
    const uint32_t dataBytes = static_cast<uint32_t>(
        voltages.size() * channels * bytesPerSample);

    stream.write("RIFF", 4);
    writeUint32(stream, 36u + dataBytes);
    stream.write("WAVE", 4);
    stream.write("fmt ", 4);
    writeUint32(stream, 16u);
    writeUint16(stream, 1u);
    writeUint16(stream, channels);
    writeUint32(stream, kSampleRate);
    writeUint32(stream, kSampleRate * channels * bytesPerSample);
    writeUint16(stream, static_cast<uint16_t>(channels * bytesPerSample));
    writeUint16(stream, bitsPerSample);
    stream.write("data", 4);
    writeUint32(stream, dataBytes);

    for (std::size_t index = 0u; index < voltages.size(); ++index) {
        const float normalized = std::max(
            -1.0f, std::min(1.0f, voltages[index] / 10.0f));
        const int16_t sample = static_cast<int16_t>(
            std::lround(normalized * 32767.0f));
        writeUint16(stream, static_cast<uint16_t>(sample));
    }
    return stream.good();
}

bool writeNormalizedAuditionWav(const std::string& path,
                                const std::vector<float>& voltages) {
    float peak = 0.0f;
    for (std::size_t index = 0u; index < voltages.size(); ++index) {
        peak = std::max(peak, std::fabs(voltages[index]));
    }
    if (peak <= 0.0f) {
        return writeMonoWav(path, voltages);
    }
    const float gain = 8.0f / peak; // -1.94 dBFS with +/-10 V WAV scaling.
    std::vector<float> normalized;
    normalized.reserve(voltages.size());
    for (std::size_t index = 0u; index < voltages.size(); ++index) {
        normalized.push_back(voltages[index] * gain);
    }
    return writeMonoWav(path, normalized);
}

burl::VoiceParameters pluginDefaults(burl::QualityMode quality) {
    burl::VoiceParameters parameters;
    parameters.oscillator1Hz = 60.0f;
    parameters.oscillator2Hz = 47.0f;
    parameters.oscillator1CrossModulation = 0.15f;
    parameters.oscillator2CrossModulation = 0.15f;
    parameters.oscillator1Feedback = 0.35f;
    parameters.oscillator2Feedback = 0.45f;
    parameters.change = 0.5f;
    parameters.filterCutoffHz = 250.0f;
    parameters.filterResonance = 0.62f;
    parameters.filterFeedback = 0.35f;
    parameters.externalCutoffModulation = 0.0f;
    parameters.externalInputMix = 0.0f;
    parameters.changeCvAmount = 0.0f;
    parameters.resonanceCvAmount = 0.0f;
    parameters.mixCvAmount = 0.0f;
    parameters.inputDrive = 1.0f;
    parameters.useExternalOscillator1Cv = false;
    parameters.useExternalOscillator2Cv = false;
    parameters.useExternalClock = false;
    parameters.doubleEdgeClock = false;
    parameters.maximal127Mode = false;
    parameters.safetyLimit = true;
    parameters.quality = quality;
    parameters.dacMsbTap = 7u;
    parameters.dacMiddleTap = 6u;
    parameters.dacLsbTap = 5u;
    parameters.auxASource = burl::AuxOscillator1Triangle;
    parameters.auxBSource = burl::AuxOscillator2Triangle;
    return parameters;
}

enum Stimulus {
    StimulusInternal,
    StimulusSine,
    StimulusSquare,
    StimulusBandlimitedSquare
};

float polyBlep(float phase, float phaseIncrement) {
    if (phase < phaseIncrement) {
        const float normalized = phase / phaseIncrement;
        return normalized + normalized - normalized * normalized - 1.0f;
    }
    if (phase > 1.0f - phaseIncrement) {
        const float normalized = (phase - 1.0f) / phaseIncrement;
        return normalized * normalized + normalized + normalized + 1.0f;
    }
    return 0.0f;
}

bool renderVoiceScenario(const std::string& directory,
                         const char* name,
                         const burl::VoiceParameters& parameters,
                         Stimulus stimulus) {
    burl::Voice voice(static_cast<float>(kSampleRate), 0x5du);
    voice.setParameters(parameters);
    voice.reset();

    const unsigned int frames = kSampleRate * kRenderSeconds;
    std::vector<float> lowPass;
    std::vector<float> bandPass;
    std::vector<float> highPass;
    std::vector<float> pwm;
    lowPass.reserve(frames);
    bandPass.reserve(frames);
    highPass.reserve(frames);
    pwm.reserve(frames);

    for (unsigned int frame = 0u; frame < frames; ++frame) {
        burl::VoiceInputs inputs;
        if (stimulus == StimulusSine) {
            inputs.filterAudio = std::sin(
                2.0 * 3.14159265358979323846 * 100.0
                * static_cast<double>(frame) / kSampleRate);
        } else if (stimulus == StimulusSquare
                   || stimulus == StimulusBandlimitedSquare) {
            const float phase = std::fmod(
                53.0f * static_cast<float>(frame) / kSampleRate, 1.0f);
            float square = phase < 0.5f ? 1.0f : -1.0f;
            if (stimulus == StimulusBandlimitedSquare) {
                const float phaseIncrement = 53.0f / kSampleRate;
                square += polyBlep(phase, phaseIncrement);
                const float secondEdge = std::fmod(phase + 0.5f, 1.0f);
                square -= polyBlep(secondEdge, phaseIncrement);
            }
            inputs.filterAudio = 5.0f * square;
        }
        const burl::VoiceOutputs output = voice.process(inputs);
        lowPass.push_back(output.lowPass);
        bandPass.push_back(output.bandPass);
        highPass.push_back(output.highPass);
        pwm.push_back(output.pwm);
    }

    const std::string prefix = directory + "/" + name;
    return writeMonoWav(prefix + "_lp.wav", lowPass)
        && writeMonoWav(prefix + "_bp.wav", bandPass)
        && writeMonoWav(prefix + "_hp.wav", highPass)
        && writeMonoWav(prefix + "_pwm.wav", pwm);
}

bool renderDefaultVoice(const std::string& directory,
                        burl::QualityMode quality,
                        const char* qualityName) {
    const std::string name = std::string("default_") + qualityName;
    return renderVoiceScenario(
        directory, name.c_str(), pluginDefaults(quality), StimulusInternal);
}

bool renderDiagnosticVariants(const std::string& directory) {
    burl::VoiceParameters parameters = pluginDefaults(burl::QualityNormal);

    parameters.filterFeedback = 0.0f;
    if (!renderVoiceScenario(directory, "variant_no_cutoff_feedback",
                             parameters, StimulusInternal)) {
        return false;
    }

    parameters = pluginDefaults(burl::QualityNormal);
    parameters.filterResonance = 0.0f;
    if (!renderVoiceScenario(directory, "variant_res0",
                             parameters, StimulusInternal)) {
        return false;
    }

    parameters = pluginDefaults(burl::QualityNormal);
    parameters.inputDrive = 0.25f;
    if (!renderVoiceScenario(directory, "variant_drive025",
                             parameters, StimulusInternal)) {
        return false;
    }

    parameters.inputDrive = 0.5f;
    if (!renderVoiceScenario(directory, "variant_drive050",
                             parameters, StimulusInternal)) {
        return false;
    }

    parameters.inputDrive = 2.0f;
    if (!renderVoiceScenario(directory, "variant_drive200",
                             parameters, StimulusInternal)) {
        return false;
    }

    parameters.inputDrive = 4.0f;
    if (!renderVoiceScenario(directory, "variant_drive400",
                             parameters, StimulusInternal)) {
        return false;
    }

    parameters = pluginDefaults(burl::QualityNormal);
    parameters.safetyLimit = false;
    if (!renderVoiceScenario(directory, "variant_limit_off",
                             parameters, StimulusInternal)) {
        return false;
    }

    parameters = pluginDefaults(burl::QualityNormal);
    parameters.externalInputMix = 1.0f;
    parameters.filterFeedback = 0.0f;
    if (!renderVoiceScenario(directory, "variant_external_sine",
                             parameters, StimulusSine)) {
        return false;
    }
    if (!renderVoiceScenario(directory, "variant_external_square",
                             parameters, StimulusSquare)) {
        return false;
    }
    return renderVoiceScenario(directory, "variant_bandlimited_square",
                               parameters, StimulusBandlimitedSquare);
}

bool renderPing(const std::string& directory, float resonance,
                const char* name) {
    burl::Voice voice(static_cast<float>(kSampleRate), 0x5du);
    burl::VoiceParameters parameters = pluginDefaults(burl::QualityNormal);
    parameters.oscillator1CrossModulation = 0.0f;
    parameters.oscillator2CrossModulation = 0.0f;
    parameters.oscillator1Feedback = 0.0f;
    parameters.oscillator2Feedback = 0.0f;
    parameters.filterCutoffHz = 250.0f;
    parameters.filterResonance = resonance;
    parameters.filterFeedback = 0.0f;
    parameters.externalInputMix = 1.0f;
    voice.setParameters(parameters);
    voice.reset();

    const unsigned int frames = kSampleRate * 2u;
    std::vector<float> lowPass;
    std::vector<float> bandPass;
    std::vector<float> highPass;
    lowPass.reserve(frames);
    bandPass.reserve(frames);
    highPass.reserve(frames);
    for (unsigned int frame = 0u; frame < frames; ++frame) {
        burl::VoiceInputs inputs;
        // Leave a half-second lead-in so the edge and full tail are visible in
        // ordinary waveform/spectrum tools and easy to audition repeatedly.
        inputs.filterAudio = frame == kSampleRate / 2u ? 5.0f : 0.0f;
        const burl::VoiceOutputs output = voice.process(inputs);
        lowPass.push_back(output.lowPass);
        bandPass.push_back(output.bandPass);
        highPass.push_back(output.highPass);
    }
    const std::string prefix = directory + "/ping_" + name;
    return writeMonoWav(prefix + "_lp_level.wav", lowPass)
        && writeMonoWav(prefix + "_bp_level.wav", bandPass)
        && writeMonoWav(prefix + "_hp_level.wav", highPass)
        && writeNormalizedAuditionWav(prefix + "_lp_audition.wav", lowPass)
        && writeNormalizedAuditionWav(prefix + "_bp_audition.wav", bandPass)
        && writeNormalizedAuditionWav(prefix + "_hp_audition.wav", highPass);
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: " << argv[0] << " OUTPUT_DIRECTORY\n";
        return EXIT_FAILURE;
    }
    const std::string directory(argv[1]);
    if (!renderDefaultVoice(directory, burl::QualityEco, "eco")
        || !renderDefaultVoice(directory, burl::QualityNormal, "normal")
        || !renderDefaultVoice(directory, burl::QualityHigh, "high")
        || !renderDiagnosticVariants(directory)
        || !renderPing(directory, 0.62f, "res62")
        || !renderPing(directory, 1.0f, "res100")) {
        return EXIT_FAILURE;
    }
    std::cout << "Wrote deterministic theoretical WAV fixtures to "
              << directory << '\n';
    return EXIT_SUCCESS;
}
