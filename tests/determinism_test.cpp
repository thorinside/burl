// Copyright (c) 2026 Burl contributors
// SPDX-License-Identifier: MIT

#include "burl/pattern_generator.hpp"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <stdint.h>
#include <vector>

namespace {

static_assert(sizeof(float) == sizeof(uint32_t),
              "determinism test requires a 32-bit float representation");

enum QualityMode {
    Eco,
    Normal,
    High
};

struct RenderContext {
    unsigned int sampleRate;
    QualityMode quality;
    burl::PatternGenerator::Mode mode;
    uint8_t seed;
};

struct RenderFrame {
    uint8_t state;
    uint32_t dacBits;
    bool registerBit;
};

struct RenderResult {
    uint8_t initialState;
    std::vector<RenderFrame> frames;
};

uint32_t floatBits(float value) {
    uint32_t bits = 0u;
    std::memcpy(&bits, &value, sizeof(bits));
    return bits;
}

unsigned int qualityOrdinal(QualityMode quality) {
    switch (quality) {
    case Eco:
        return 0u;
    case Normal:
        return 1u;
    case High:
        return 2u;
    }
    return 0u;
}

const char* qualityName(QualityMode quality) {
    switch (quality) {
    case Eco:
        return "Eco";
    case Normal:
        return "Normal";
    case High:
        return "High";
    }
    return "unknown";
}

unsigned int sequenceValue(const RenderContext& context, unsigned int index,
                           unsigned int salt) {
    return index * 37u + context.sampleRate / 100u
        + qualityOrdinal(context.quality) * 53u + salt;
}

float changeAt(const RenderContext& context, unsigned int index) {
    const unsigned int value = sequenceValue(context, index, 11u) % 101u;
    return static_cast<float>(value) / 100.0f;
}

float chaosInputAt(const RenderContext& context, unsigned int index) {
    const unsigned int value = sequenceValue(context, index, 29u) % 201u;
    return static_cast<float>(static_cast<int>(value) - 100) / 100.0f;
}

RenderResult render(const RenderContext& context) {
    burl::PatternGenerator generator(context.seed);
    generator.setMode(context.mode);

    // Establish a non-trivial running state independently for each render.
    // The resulting state is captured so the comparison verifies both runs
    // started the measured interval from an identical initial state.
    for (unsigned int index = 0u; index < 47u; ++index) {
        generator.clock(changeAt(context, index), chaosInputAt(context, index));
    }

    RenderResult result;
    result.initialState = generator.state();
    result.frames.reserve(1024u);

    for (unsigned int index = 0u; index < 1024u; ++index) {
        const unsigned int streamIndex = index + 47u;
        generator.clock(changeAt(context, streamIndex),
                        chaosInputAt(context, streamIndex));

        RenderFrame frame;
        frame.state = generator.state();
        frame.dacBits = floatBits(generator.dac(
            streamIndex % 8u, (streamIndex + 3u) % 8u,
            (streamIndex + 5u) % 8u));
        frame.registerBit = generator.bit((streamIndex + 1u) % 8u);
        result.frames.push_back(frame);
    }

    return result;
}

bool framesEqual(const RenderFrame& first, const RenderFrame& second) {
    return first.state == second.state
        && first.dacBits == second.dacBits
        && first.registerBit == second.registerBit;
}

int testRepeatedRendersAreBitExact() {
    const unsigned int sampleRates[] = {32000u, 44100u, 48000u, 88200u, 96000u};
    const QualityMode qualities[] = {Eco, Normal, High};
    const burl::PatternGenerator::Mode modes[] = {
        burl::PatternGenerator::EightSixteen,
        burl::PatternGenerator::Maximal127
    };

    int failures = 0;
    for (unsigned int rateIndex = 0u; rateIndex < 5u; ++rateIndex) {
        for (unsigned int qualityIndex = 0u; qualityIndex < 3u; ++qualityIndex) {
            for (unsigned int modeIndex = 0u; modeIndex < 2u; ++modeIndex) {
                RenderContext context;
                context.sampleRate = sampleRates[rateIndex];
                context.quality = qualities[qualityIndex];
                context.mode = modes[modeIndex];
                context.seed = static_cast<uint8_t>(
                    0x35u + rateIndex * 17u + qualityIndex * 31u + modeIndex * 7u);

                const RenderResult first = render(context);
                const RenderResult second = render(context);

                if (first.initialState != second.initialState) {
                    std::cerr << "FAIL: initial state differs at "
                              << context.sampleRate << " Hz, "
                              << qualityName(context.quality) << " quality\n";
                    ++failures;
                    continue;
                }

                if (first.frames.size() != second.frames.size()
                    || first.frames.empty()) {
                    std::cerr << "FAIL: render length differs or is empty at "
                              << context.sampleRate << " Hz, "
                              << qualityName(context.quality) << " quality\n";
                    ++failures;
                    continue;
                }

                for (std::size_t frame = 0u; frame < first.frames.size(); ++frame) {
                    if (!framesEqual(first.frames[frame], second.frames[frame])) {
                        std::cerr << "FAIL: render differs at frame " << frame
                                  << ", " << context.sampleRate << " Hz, "
                                  << qualityName(context.quality) << " quality\n";
                        ++failures;
                        break;
                    }
                }
            }
        }
    }

    return failures;
}

} // namespace

int main() {
    const int failures = testRepeatedRendersAreBitExact();
    if (failures != 0) {
        std::cerr << failures << " deterministic-render check(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "All deterministic pattern-render tests passed\n";
    return EXIT_SUCCESS;
}
