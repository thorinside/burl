// Copyright (c) 2026 Burl contributors
// SPDX-License-Identifier: MIT

#include "burl/filter.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <stdint.h>
#include <string>
#include <vector>

namespace {

const unsigned int kReferenceFactor = 16u;
const unsigned int kWarmupSeconds = 1u;
const unsigned int kRenderSeconds = 4u;
const unsigned int kFirTaps = 511u;
const double kPi = 3.1415926535897932384626433832795;
const double kV1WorstCaseForcing = 0.330515;

struct Frame {
    double lowPass;
    double bandPass;
    double highPass;
};

class DoubleReferenceFilter {
public:
    DoubleReferenceFilter()
        : integrator1_(0.0), integrator2_(0.0), previousBand_(0.0) {}

    Frame process(double input, double cutoffHz, double resonance,
                  double sampleRate) {
        const double damping = 2.0 * std::pow(40.0, -resonance);
        const double inputGain = 1.0 - 0.25 * resonance * resonance;
        const double skewOctaves = 0.5 * previousBand_;
        const double cutoff = std::max(
            1.0, std::min(sampleRate * 0.45,
                          cutoffHz * std::pow(2.0, skewOctaves)));
        const double g = std::tan(kPi * cutoff / sampleRate);
        const double a1 = 1.0 / (1.0 + g * (g + damping));
        const double a2 = g * a1;
        const double a3 = g * a2;
        const double conditionedInput = input * inputGain;
        const double v3 = conditionedInput - integrator2_;
        const double bandPass = a1 * integrator1_ + a2 * v3;
        const double lowPass = integrator2_
            + a2 * integrator1_ + a3 * v3;
        const double highPass = conditionedInput
            - damping * bandPass - lowPass;

        integrator1_ = 2.0 * bandPass - integrator1_;
        integrator2_ = 2.0 * lowPass - integrator2_;
        previousBand_ = bandPass;
        Frame frame = { lowPass, bandPass, highPass };
        return frame;
    }

private:
    double integrator1_;
    double integrator2_;
    double previousBand_;
};

enum Stimulus {
    StimulusSine,
    StimulusSquare
};

double stimulusAt(Stimulus stimulus, uint64_t sample,
                  unsigned int sampleRate) {
    const double phase = 53.0 * static_cast<double>(sample)
        / static_cast<double>(sampleRate);
    if (stimulus == StimulusSine) {
        return kV1WorstCaseForcing * std::sin(2.0 * kPi * phase);
    }
    return std::fmod(phase, 1.0) < 0.5
        ? kV1WorstCaseForcing : -kV1WorstCaseForcing;
}

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

void writeUint24(std::ofstream& stream, uint32_t value) {
    const char bytes[] = {
        static_cast<char>(value & 0xffu),
        static_cast<char>((value >> 8u) & 0xffu),
        static_cast<char>((value >> 16u) & 0xffu)
    };
    stream.write(bytes, sizeof(bytes));
}

bool writeMonoWav(const std::string& path,
                  const std::vector<double>& voltages,
                  unsigned int sampleRate) {
    std::ofstream stream(path.c_str(), std::ios::out | std::ios::binary);
    if (!stream) {
        std::cerr << "could not create " << path << '\n';
        return false;
    }

    const uint16_t channels = 1u;
    const uint16_t bitsPerSample = 24u;
    const uint32_t bytesPerSample = bitsPerSample / 8u;
    const uint32_t dataBytes = static_cast<uint32_t>(
        voltages.size() * bytesPerSample);
    stream.write("RIFF", 4);
    writeUint32(stream, 36u + dataBytes);
    stream.write("WAVE", 4);
    stream.write("fmt ", 4);
    writeUint32(stream, 16u);
    writeUint16(stream, 1u);
    writeUint16(stream, channels);
    writeUint32(stream, sampleRate);
    writeUint32(stream, sampleRate * bytesPerSample);
    writeUint16(stream, static_cast<uint16_t>(bytesPerSample));
    writeUint16(stream, bitsPerSample);
    stream.write("data", 4);
    writeUint32(stream, dataBytes);

    for (std::size_t index = 0u; index < voltages.size(); ++index) {
        const double normalized = std::max(
            -1.0, std::min(1.0, voltages[index] / 10.0));
        const int32_t sample = static_cast<int32_t>(
            std::llround(normalized * 8388607.0));
        writeUint24(stream, static_cast<uint32_t>(sample));
    }
    return stream.good();
}

std::vector<double> makeFir(unsigned int outputSampleRate) {
    std::vector<double> taps(kFirTaps, 0.0);
    const int half = static_cast<int>(kFirTaps / 2u);
    const unsigned int referenceSampleRate =
        outputSampleRate * kReferenceFactor;
    const double cutoffHz = std::min(
        20000.0, 0.45 * static_cast<double>(outputSampleRate));
    const double normalizedCutoff = cutoffHz
        / static_cast<double>(referenceSampleRate);
    double sum = 0.0;
    for (int offset = -half; offset <= half; ++offset) {
        const double ideal = offset == 0
            ? 2.0 * normalizedCutoff
            : std::sin(2.0 * kPi * normalizedCutoff * offset)
                / (kPi * offset);
        const double position = static_cast<double>(offset + half)
            / static_cast<double>(kFirTaps - 1u);
        const double window = 0.42 - 0.5 * std::cos(2.0 * kPi * position)
            + 0.08 * std::cos(4.0 * kPi * position);
        taps[static_cast<std::size_t>(offset + half)] = ideal * window;
        sum += ideal * window;
    }
    for (std::size_t index = 0u; index < taps.size(); ++index) {
        taps[index] /= sum;
    }
    return taps;
}

std::vector<double> decimate(const std::vector<double>& highRate,
                             const std::vector<double>& fir,
                             unsigned int outputSampleRate) {
    const unsigned int outputFrames = outputSampleRate * kRenderSeconds;
    const unsigned int half = kFirTaps / 2u;
    const uint64_t firstCenter =
        static_cast<uint64_t>(kWarmupSeconds)
            * outputSampleRate * kReferenceFactor
        + kReferenceFactor / 2u;
    std::vector<double> output(outputFrames, 0.0);
    for (unsigned int frame = 0u; frame < outputFrames; ++frame) {
        const uint64_t center = firstCenter
            + static_cast<uint64_t>(frame) * kReferenceFactor;
        double sample = 0.0;
        for (unsigned int tap = 0u; tap < kFirTaps; ++tap) {
            const uint64_t source = center + tap - half;
            sample += fir[tap] * highRate[static_cast<std::size_t>(source)];
        }
        output[frame] = sample;
    }
    return output;
}

bool renderReference(const std::string& directory, Stimulus stimulus,
                     const char* stimulusName,
                     unsigned int outputSampleRate) {
    const unsigned int referenceSampleRate =
        outputSampleRate * kReferenceFactor;
    const unsigned int half = kFirTaps / 2u;
    const uint64_t highFrames =
        static_cast<uint64_t>(kWarmupSeconds + kRenderSeconds)
            * referenceSampleRate
        + half + kReferenceFactor;
    std::vector<double> lowPass;
    std::vector<double> bandPass;
    std::vector<double> highPass;
    lowPass.reserve(static_cast<std::size_t>(highFrames));
    bandPass.reserve(static_cast<std::size_t>(highFrames));
    highPass.reserve(static_cast<std::size_t>(highFrames));

    DoubleReferenceFilter filter;
    for (uint64_t frame = 0u; frame < highFrames; ++frame) {
        const Frame output = filter.process(
            stimulusAt(stimulus, frame, referenceSampleRate),
            250.0, 0.62, static_cast<double>(referenceSampleRate));
        lowPass.push_back(output.lowPass);
        bandPass.push_back(output.bandPass);
        highPass.push_back(output.highPass);
    }

    const std::vector<double> fir = makeFir(outputSampleRate);
    const std::string prefix = directory + "/sr"
        + std::to_string(outputSampleRate) + "_reference16x_"
        + stimulusName;
    return writeMonoWav(prefix + "_lp.wav",
                        decimate(lowPass, fir, outputSampleRate),
                        outputSampleRate)
        && writeMonoWav(prefix + "_bp.wav",
                        decimate(bandPass, fir, outputSampleRate),
                        outputSampleRate)
        && writeMonoWav(prefix + "_hp.wav",
                        decimate(highPass, fir, outputSampleRate),
                        outputSampleRate);
}

bool renderProduction(const std::string& directory, Stimulus stimulus,
                      const char* stimulusName, unsigned int factor,
                      const char* qualityName,
                      unsigned int outputSampleRate) {
    const unsigned int internalSampleRate = outputSampleRate * factor;
    const unsigned int totalFrames =
        outputSampleRate * (kWarmupSeconds + kRenderSeconds);
    const unsigned int captureStart = outputSampleRate * kWarmupSeconds;
    std::vector<double> lowPass;
    std::vector<double> bandPass;
    std::vector<double> highPass;
    lowPass.reserve(outputSampleRate * kRenderSeconds);
    bandPass.reserve(outputSampleRate * kRenderSeconds);
    highPass.reserve(outputSampleRate * kRenderSeconds);

    burl::StateVariableFilter filter;
    uint64_t internalFrame = 0u;
    for (unsigned int frame = 0u; frame < totalFrames; ++frame) {
        double low = 0.0;
        double band = 0.0;
        double high = 0.0;
        for (unsigned int substep = 0u; substep < factor;
             ++substep, ++internalFrame) {
            const burl::StateVariableFilter::Frame output = filter.process(
                static_cast<float>(stimulusAt(
                    stimulus, internalFrame, internalSampleRate)),
                250.0f, 0.0f, 0.62f,
                static_cast<float>(internalSampleRate), false);
            low += output.lowPass;
            band += output.bandPass;
            high += output.highPass;
        }
        if (frame >= captureStart) {
            const double inverseFactor = 1.0 / factor;
            lowPass.push_back(low * inverseFactor);
            bandPass.push_back(band * inverseFactor);
            highPass.push_back(high * inverseFactor);
        }
    }

    const std::string prefix = directory + "/sr"
        + std::to_string(outputSampleRate) + "_production_"
        + qualityName + "_" + stimulusName;
    return writeMonoWav(prefix + "_lp.wav", lowPass, outputSampleRate)
        && writeMonoWav(prefix + "_bp.wav", bandPass, outputSampleRate)
        && writeMonoWav(prefix + "_hp.wav", highPass, outputSampleRate);
}

bool renderStimulus(const std::string& directory, Stimulus stimulus,
                    const char* name, unsigned int outputSampleRate) {
    return renderReference(directory, stimulus, name, outputSampleRate)
        && renderProduction(directory, stimulus, name, 1u, "eco",
                            outputSampleRate)
        && renderProduction(directory, stimulus, name, 2u, "normal",
                            outputSampleRate)
        && renderProduction(directory, stimulus, name, 4u, "high",
                            outputSampleRate);
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: " << argv[0] << " OUTPUT_DIRECTORY\n";
        return EXIT_FAILURE;
    }
    const std::string directory(argv[1]);
    const unsigned int sampleRates[] = {
        32000u, 44100u, 48000u, 88200u, 96000u
    };
    for (std::size_t index = 0u;
         index < sizeof(sampleRates) / sizeof(sampleRates[0]); ++index) {
        if (!renderStimulus(directory, StimulusSine, "sine",
                            sampleRates[index])
            || !renderStimulus(directory, StimulusSquare, "square",
                               sampleRates[index])) {
            return EXIT_FAILURE;
        }
    }
    std::cout << "Wrote five-rate 16x double/FIR reference WAVs to "
              << directory << '\n';
    return EXIT_SUCCESS;
}
