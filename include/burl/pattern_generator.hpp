// Copyright (c) 2026 Burl contributors
// SPDX-License-Identifier: MIT

#ifndef BURL_PATTERN_GENERATOR_HPP
#define BURL_PATTERN_GENERATOR_HPP

#include <stdint.h>

namespace burl {

/**
 * Deterministic eight-bit feedback shift-register pattern generator.
 *
 * The EightSixteen mode recirculates the outgoing bit at the low Change
 * endpoint and recirculates its complement at the high endpoint. The
 * Maximal127 mode uses an isolated seven-bit maximal-length recurrence.
 * This class owns no dynamic memory and is suitable for the audio callback.
 */
class PatternGenerator {
public:
    enum Mode {
        EightSixteen,
        Maximal127
    };

    explicit PatternGenerator(uint8_t seed = 1u);

    void setMode(Mode mode);
    Mode mode() const;

    void setSeed(uint8_t seed);
    uint8_t seed() const;
    void reset();

    /**
     * Advance one shift.
     *
     * change is clamped to [0, 1]. chaosSignal is clamped to [-1, 1] and
     * supplies the deterministic decision between direct and complemented
     * recirculation away from the endpoints. It is ignored in Maximal127.
     */
    void clock(float change, float chaosSignal);

    uint8_t state() const;
    bool bit(unsigned int tap) const;

    /**
     * Convert three selected register taps to an exact bipolar eight-level
     * DAC value. msbTap carries weight four; lsbTap carries weight one.
     */
    float dac(unsigned int msbTap, unsigned int middleTap,
              unsigned int lsbTap) const;

    static float dacLevel(unsigned int code);

private:
    static uint8_t nonZero127Seed(uint8_t value);

    Mode mode_;
    uint8_t configuredSeed_;
    uint8_t state_;
};

} // namespace burl

#endif // BURL_PATTERN_GENERATOR_HPP
