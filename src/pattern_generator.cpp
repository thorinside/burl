// Copyright (c) 2026 Burl contributors
// SPDX-License-Identifier: MIT

#include "burl/pattern_generator.hpp"

namespace burl {
namespace {

float clamp(float value, float low, float high) {
    return value < low ? low : (value > high ? high : value);
}

} // namespace

PatternGenerator::PatternGenerator(uint8_t seed)
    : mode_(EightSixteen), configuredSeed_(seed), state_(seed) {}

void PatternGenerator::setMode(Mode mode) {
    mode_ = mode;
    if (mode_ == Maximal127) {
        state_ = nonZero127Seed(state_);
    }
}

PatternGenerator::Mode PatternGenerator::mode() const {
    return mode_;
}

void PatternGenerator::setSeed(uint8_t seed) {
    configuredSeed_ = seed;
}

uint8_t PatternGenerator::seed() const {
    return configuredSeed_;
}

void PatternGenerator::reset() {
    state_ = mode_ == Maximal127
        ? nonZero127Seed(configuredSeed_)
        : configuredSeed_;
}

void PatternGenerator::clock(float change, float chaosSignal) {
    if (mode_ == Maximal127) {
        // x^7 + x^6 + 1, shifted right. Every non-zero seven-bit seed
        // traverses all 127 non-zero states before repeating.
        const uint8_t state = nonZero127Seed(state_);
        const uint8_t feedback = static_cast<uint8_t>((state ^ (state >> 1u)) & 1u);
        state_ = static_cast<uint8_t>(((state >> 1u) | (feedback << 6u)) & 0x7fu);
        return;
    }

    const uint8_t outgoing = static_cast<uint8_t>(state_ & 1u);
    bool complement;
    if (change <= 0.0f) {
        complement = false;
    } else if (change >= 1.0f) {
        complement = true;
    } else {
        const float decision = 0.5f * (clamp(chaosSignal, -1.0f, 1.0f) + 1.0f);
        complement = decision < change;
    }

    const uint8_t incoming = static_cast<uint8_t>(outgoing ^ (complement ? 1u : 0u));
    state_ = static_cast<uint8_t>((state_ >> 1u) | (incoming << 7u));
}

uint8_t PatternGenerator::state() const {
    return state_;
}

bool PatternGenerator::bit(unsigned int tap) const {
    return ((state_ >> (tap & 7u)) & 1u) != 0u;
}

float PatternGenerator::dac(unsigned int msbTap, unsigned int middleTap,
                            unsigned int lsbTap) const {
    const unsigned int code = (bit(msbTap) ? 4u : 0u)
        | (bit(middleTap) ? 2u : 0u)
        | (bit(lsbTap) ? 1u : 0u);
    return dacLevel(code);
}

float PatternGenerator::dacLevel(unsigned int code) {
    const unsigned int boundedCode = code > 7u ? 7u : code;
    return -5.0f + static_cast<float>(boundedCode) * (10.0f / 7.0f);
}

uint8_t PatternGenerator::nonZero127Seed(uint8_t value) {
    const uint8_t sevenBit = static_cast<uint8_t>(value & 0x7fu);
    return sevenBit == 0u ? 1u : sevenBit;
}

} // namespace burl
