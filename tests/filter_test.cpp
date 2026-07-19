// Copyright (c) 2026 Burl contributors
// SPDX-License-Identifier: MIT

#include "burl/filter.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <vector>

namespace {

const float kPi = 3.14159265358979323846f;
const float kSampleRate = 96000.0f;
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

void testCurvedResonanceMapping() {
    expectNear(1.0f / burl::StateVariableFilter::resonanceDamping(0.0f),
               0.5f, 0.000001f, "minimum resonance must map to Q 0.5");
    expectNear(1.0f / burl::StateVariableFilter::resonanceDamping(0.5f),
               3.16227766f, 0.00001f,
               "midpoint resonance must follow the exponential Q curve");
    expectNear(1.0f / burl::StateVariableFilter::resonanceDamping(1.0f),
               20.0f, 0.00001f, "maximum resonance must map to Q 20");

    float previousQ = 0.0f;
    for (unsigned int step = 0u; step <= 100u; ++step) {
        const float resonance = static_cast<float>(step) / 100.0f;
        const float q = 1.0f
            / burl::StateVariableFilter::resonanceDamping(resonance);
        expect(std::isfinite(q) && q >= previousQ,
               "resonance Q lookup must remain finite and monotonic");
        previousQ = q;
    }
}

void testNonFiniteResonanceIsSafe() {
    const float infinity = std::numeric_limits<float>::infinity();
    const float nan = std::numeric_limits<float>::quiet_NaN();
    expectNear(burl::StateVariableFilter::resonanceDamping(nan), 2.0f,
               0.0f, "NaN resonance must fall back to minimum resonance");
    expectNear(burl::StateVariableFilter::resonanceDamping(infinity), 0.05f,
               0.0f, "positive infinity resonance must saturate at maximum");
    expectNear(burl::StateVariableFilter::resonanceDamping(-infinity), 2.0f,
               0.0f, "negative infinity resonance must saturate at minimum");

    burl::StateVariableFilter filter;
    const burl::StateVariableFilter::Frame invalid = filter.process(
        1.0f, 250.0f, 0.0f, nan, kSampleRate, false);
    expect(std::isfinite(invalid.lowPass)
               && std::isfinite(invalid.bandPass)
               && std::isfinite(invalid.highPass),
           "NaN resonance must not produce non-finite filter output");

    const burl::StateVariableFilter::Frame recovered = filter.process(
        1.0f, 250.0f, 0.0f, 0.62f, kSampleRate, false);
    expect(std::isfinite(recovered.lowPass)
               && std::isfinite(recovered.bandPass)
               && std::isfinite(recovered.highPass),
           "filter must recover after a non-finite resonance value");
}

float impulseDecayMilliseconds(float resonance) {
    burl::StateVariableFilter filter;
    const unsigned int frames = static_cast<unsigned int>(kSampleRate * 2.0f);
    std::vector<float> bandPass;
    bandPass.reserve(frames);
    float peak = 0.0f;
    for (unsigned int frame = 0u; frame < frames; ++frame) {
        const float input = frame == 0u ? 5.0f : 0.0f;
        const burl::StateVariableFilter::Frame output = filter.process(
            input, 250.0f, 0.0f, resonance, kSampleRate, false);
        const float magnitude = std::fabs(output.bandPass);
        bandPass.push_back(magnitude);
        if (magnitude > peak) {
            peak = magnitude;
        }
    }

    const float threshold = peak * 0.001f;
    unsigned int lastAboveThreshold = 0u;
    for (unsigned int frame = 0u; frame < frames; ++frame) {
        if (bandPass[frame] >= threshold) {
            lastAboveThreshold = frame;
        }
    }
    return 1000.0f * static_cast<float>(lastAboveThreshold) / kSampleRate;
}

void testPingDecayAndNoSelfOscillation() {
    expectNear(impulseDecayMilliseconds(0.62f), 42.5f, 3.0f,
               "default resonance must produce the intended 250 Hz ping tail");
    expectNear(impulseDecayMilliseconds(1.0f), 174.3f, 10.0f,
               "maximum resonance must ring strongly without becoming infinite");

    burl::StateVariableFilter silent;
    for (unsigned int frame = 0u; frame < 96000u; ++frame) {
        const burl::StateVariableFilter::Frame output = silent.process(
            0.0f, 250.0f, 0.0f, 1.0f, kSampleRate, false);
        expect(output.lowPass == 0.0f && output.bandPass == 0.0f
                   && output.highPass == 0.0f,
               "maximum resonance must not begin oscillating from silence");
        if (failures != 0) {
            break;
        }
    }

    burl::StateVariableFilter excited;
    burl::StateVariableFilter::Frame output = excited.process(
        5.0f, 250.0f, 0.0f, 1.0f, kSampleRate, false);
    for (unsigned int frame = 1u; frame < 192000u; ++frame) {
        output = excited.process(
            0.0f, 250.0f, 0.0f, 1.0f, kSampleRate, false);
    }
    expect(std::fabs(output.lowPass) < 0.000001f
               && std::fabs(output.bandPass) < 0.000001f
               && std::fabs(output.highPass) < 0.000001f,
           "an excited maximum-resonance filter must decay back to silence");

    burl::StateVariableFilter energyDecay;
    energyDecay.process(5.0f, 250.0f, 0.0f, 1.0f, kSampleRate, false);
    double previousEnergy = 0.0;
    const unsigned int windowFrames = 3840u; // Ten cycles at 250 Hz.
    for (unsigned int window = 0u; window < 50u; ++window) {
        double energy = 0.0;
        for (unsigned int frame = 0u; frame < windowFrames; ++frame) {
            const burl::StateVariableFilter::Frame decay = energyDecay.process(
                0.0f, 250.0f, 0.0f, 1.0f, kSampleRate, false);
            energy += static_cast<double>(decay.lowPass) * decay.lowPass
                + static_cast<double>(decay.bandPass) * decay.bandPass;
        }
        if (window > 0u) {
            expect(energy <= previousEnergy,
                   "maximum-resonance windowed energy must always decrease");
        }
        previousEnergy = energy;
    }
}

void testDcCoupledOutputs() {
    burl::StateVariableFilter filter;
    burl::StateVariableFilter::Frame output = filter.process(
        1.0f, 250.0f, 0.0f, 0.0f, kSampleRate, false);
    for (unsigned int frame = 1u; frame < 96000u; ++frame) {
        output = filter.process(
            1.0f, 250.0f, 0.0f, 0.0f, kSampleRate, false);
    }
    expectNear(output.lowPass, 1.0f, 0.0001f,
               "low-pass output must retain steady DC");
    expectNear(output.bandPass, 0.0f, 0.0001f,
               "band-pass output must settle after a steady DC input");
    expectNear(output.highPass, 0.0f, 0.0001f,
               "high-pass output must settle after a steady DC input");

    for (unsigned int frame = 0u; frame < 96000u; ++frame) {
        output = filter.process(
            -0.5f, 250.0f, 0.0f, 0.0f, kSampleRate, false);
    }
    expectNear(output.lowPass, -0.5f, 0.0001f,
               "low-pass output must retain stepped DC after settling");
    expectNear(output.bandPass, 0.0f, 0.0001f,
               "band-pass output must reject settled stepped DC");
    expectNear(output.highPass, 0.0f, 0.0001f,
               "high-pass output must reject settled stepped DC");
}

float sineRms(float amplitude) {
    burl::StateVariableFilter filter;
    double squared = 0.0;
    unsigned int count = 0u;
    for (unsigned int frame = 0u; frame < 96000u; ++frame) {
        const float input = amplitude * std::sin(
            2.0f * kPi * 100.0f * static_cast<float>(frame) / kSampleRate);
        const burl::StateVariableFilter::Frame output = filter.process(
            input, 1000.0f, 0.0f, 0.0f, kSampleRate, false);
        if (frame >= 48000u) {
            squared += static_cast<double>(output.lowPass)
                * static_cast<double>(output.lowPass);
            ++count;
        }
    }
    return static_cast<float>(std::sqrt(squared / count));
}

void testTransparentInputProtection() {
    const float lowQuarter = sineRms(0.125f);
    const float lowHalf = sineRms(0.25f);
    const float lowOne = sineRms(0.5f);
    const float lowTwo = sineRms(1.0f);
    expectNear(lowHalf / lowQuarter, 2.0f, 0.001f,
               "low-level 0.25x to 0.5x drive must scale linearly");
    expectNear(lowOne / lowHalf, 2.0f, 0.001f,
               "low-level 0.5x to 1x drive must scale linearly");
    expectNear(lowTwo / lowOne, 2.0f, 0.001f,
               "low-level 1x to 2x drive must scale linearly");

    const float normalOne = sineRms(5.0f);
    const float normalTwo = sineRms(10.0f);
    const float normalFour = sineRms(20.0f);
    expect(normalOne < normalTwo && normalTwo < normalFour,
           "input protection must remain monotonic through its full range");
    expect(normalFour < normalTwo * 1.75f,
           "out-of-range input must enter progressive protection instead of plain gain");
}

void testOutputNormalizationPrecedesLimiter() {
    burl::StateVariableFilter unbounded;
    burl::StateVariableFilter limited;
    burl::StateVariableFilter::Frame unboundedOutput = unbounded.process(
        1.0f, 1000.0f, 0.0f, 0.0f, kSampleRate, false, 10.0f);
    burl::StateVariableFilter::Frame limitedOutput = limited.process(
        1.0f, 1000.0f, 0.0f, 0.0f, kSampleRate, true, 10.0f);
    for (unsigned int frame = 1u; frame < 96000u; ++frame) {
        unboundedOutput = unbounded.process(
            1.0f, 1000.0f, 0.0f, 0.0f, kSampleRate, false, 10.0f);
        limitedOutput = limited.process(
            1.0f, 1000.0f, 0.0f, 0.0f, kSampleRate, true, 10.0f);
    }
    expectNear(unboundedOutput.lowPass, 10.0f, 0.001f,
               "output normalization must provide the requested final gain");
    expectNear(limitedOutput.lowPass, 9.0f, 0.001f,
               "soft limiting must follow final output normalization");
}

void testOutputNormalizationDoesNotFeedBackIntoCharacter() {
    burl::StateVariableFilter core;
    burl::StateVariableFilter normalized;
    for (unsigned int frame = 0u; frame < 8192u; ++frame) {
        const float input = (frame % 173u) < 71u ? 0.2f : -0.2f;
        const burl::StateVariableFilter::Frame coreOutput = core.process(
            input, 317.0f, 0.13f, 0.81f, kSampleRate, false, 1.0f);
        const burl::StateVariableFilter::Frame normalizedOutput =
            normalized.process(
                input, 317.0f, 0.13f, 0.81f, kSampleRate, false, 10.0f);
        expectNear(normalizedOutput.lowPass, 10.0f * coreOutput.lowPass,
                   0.000001f,
                   "output normalization must not alter LP core state");
        expectNear(normalizedOutput.bandPass, 10.0f * coreOutput.bandPass,
                   0.000001f,
                   "output normalization must not alter BP core state");
        expectNear(normalizedOutput.highPass, 10.0f * coreOutput.highPass,
                   0.000001f,
                   "output normalization must not alter HP core state");
        if (failures != 0) {
            break;
        }
    }
}

void testV1NetworkSquareWaveHeadroom() {
    burl::StateVariableFilter filter;
    const unsigned int warmupFrames = 24000u;
    const unsigned int measurementFrames = 480000u;
    unsigned int lowPassLimited = 0u;
    unsigned int bandPassLimited = 0u;
    unsigned int highPassLimited = 0u;
    for (unsigned int frame = 0u;
         frame < warmupFrames + measurementFrames; ++frame) {
        const float phase = std::fmod(
            53.0f * static_cast<float>(frame) / kSampleRate, 1.0f);
        // Worst-case same-sign PWM and RUNCV forcing from the published V1
        // resistor network, converted to Burl's +/-5 V source ranges.
        const float input = phase < 0.5f ? 0.330515f : -0.330515f;
        const burl::StateVariableFilter::Frame output = filter.process(
            input, 250.0f, 0.0f, 0.62f, kSampleRate, true);
        if (frame < warmupFrames) {
            continue;
        }
        lowPassLimited += std::fabs(output.lowPass) > 8.0f ? 1u : 0u;
        bandPassLimited += std::fabs(output.bandPass) > 8.0f ? 1u : 0u;
        highPassLimited += std::fabs(output.highPass) > 8.0f ? 1u : 0u;
    }

    const unsigned int maximumLimitedFrames = measurementFrames / 100u;
    expect(lowPassLimited <= maximumLimitedFrames,
           "V1-network square-wave LP must not live in the output limiter");
    expect(bandPassLimited <= maximumLimitedFrames,
           "V1-network square-wave BP must not live in the output limiter");
    expect(highPassLimited <= maximumLimitedFrames,
           "V1-network square-wave HP must not live in the output limiter");
}

void testHighResonanceAllHarmonicCharacter() {
    burl::StateVariableFilter filter;
    const float frequency = 250.0f;
    const unsigned int warmupFrames = 144000u;
    const unsigned int captureFrames = 48000u;
    double real[3] = {0.0, 0.0, 0.0};
    double imaginary[3] = {0.0, 0.0, 0.0};

    for (unsigned int frame = 0u;
         frame < warmupFrames + captureFrames; ++frame) {
        const float input = 0.5f * std::sin(
            2.0f * kPi * frequency * static_cast<float>(frame) / kSampleRate);
        const burl::StateVariableFilter::Frame output = filter.process(
            input, frequency, 0.0f, 1.0f, kSampleRate, false);
        if (frame < warmupFrames) {
            continue;
        }
        const unsigned int captureFrame = frame - warmupFrames;
        for (unsigned int harmonic = 1u; harmonic <= 3u; ++harmonic) {
            const double phase = 2.0 * static_cast<double>(kPi)
                * static_cast<double>(harmonic) * frequency
                * static_cast<double>(captureFrame) / kSampleRate;
            real[harmonic - 1u] += output.bandPass * std::cos(phase);
            imaginary[harmonic - 1u] -= output.bandPass * std::sin(phase);
        }
    }

    float magnitudes[3];
    for (unsigned int harmonic = 0u; harmonic < 3u; ++harmonic) {
        magnitudes[harmonic] = static_cast<float>(
            2.0 * std::sqrt(real[harmonic] * real[harmonic]
                            + imaginary[harmonic] * imaginary[harmonic])
            / captureFrames);
    }
    const float secondDb = 20.0f * std::log10(magnitudes[1] / magnitudes[0]);
    const float thirdDb = 20.0f * std::log10(magnitudes[2] / magnitudes[0]);
    expect(secondDb >= -22.0f && secondDb <= -10.0f,
           "maximum-resonance skew must create controlled even harmonics");
    expect(thirdDb >= -40.0f && thirdDb <= -20.0f,
           "maximum-resonance skew must create controlled odd harmonics");
}

} // namespace

int main() {
    testCurvedResonanceMapping();
    testNonFiniteResonanceIsSafe();
    testPingDecayAndNoSelfOscillation();
    testDcCoupledOutputs();
    testTransparentInputProtection();
    testOutputNormalizationPrecedesLimiter();
    testOutputNormalizationDoesNotFeedBackIntoCharacter();
    testV1NetworkSquareWaveHeadroom();
    testHighResonanceAllHarmonicCharacter();

    if (failures != 0) {
        std::cerr << failures << " filter assertion(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "All filter ping and character tests passed\n";
    return EXIT_SUCCESS;
}
