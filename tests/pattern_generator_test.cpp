// Copyright (c) 2026 Burl contributors
// SPDX-License-Identifier: MIT

#include "burl/pattern_generator.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>

namespace {

int failures = 0;

void expect(bool condition, const char* message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

void expectNear(float actual, float expected, float tolerance, const char* message) {
    if (std::fabs(actual - expected) > tolerance) {
        std::cerr << "FAIL: " << message << " (expected " << expected
                  << ", got " << actual << ")\n";
        ++failures;
    }
}

void testDirectRecirculationReturnsAfterEightShifts() {
    for (unsigned int seed = 0u; seed <= 255u; ++seed) {
        burl::PatternGenerator generator(static_cast<uint8_t>(seed));
        for (int shift = 0; shift < 8; ++shift) {
            generator.clock(0.0f, shift % 2 == 0 ? -1.0f : 1.0f);
        }
        expect(generator.state() == seed,
               "low Change endpoint must return every byte after eight shifts");
    }
}

void testComplementedRecirculationHasSixteenShiftCycle() {
    for (unsigned int seed = 0u; seed <= 255u; ++seed) {
        burl::PatternGenerator generator(static_cast<uint8_t>(seed));
        for (int shift = 0; shift < 8; ++shift) {
            generator.clock(1.0f, shift % 2 == 0 ? -1.0f : 1.0f);
        }
        expect(generator.state() == static_cast<uint8_t>(~seed),
               "high Change endpoint must complement every byte after eight shifts");
        for (int shift = 0; shift < 8; ++shift) {
            generator.clock(1.0f, 0.0f);
        }
        expect(generator.state() == seed,
               "high Change endpoint must return every byte after sixteen shifts");
    }
}

void testMaximalRecurrenceVisits127NonZeroStates() {
    for (unsigned int seed = 1u; seed <= 127u; ++seed) {
        burl::PatternGenerator generator(static_cast<uint8_t>(seed));
        generator.setMode(burl::PatternGenerator::Maximal127);
        const uint8_t initial = generator.state();
        bool visited[128] = {};

        for (int shift = 0; shift < 127; ++shift) {
            const uint8_t state = generator.state();
            expect(state != 0u, "127 recurrence must never enter zero");
            expect(!visited[state], "127 recurrence must not repeat before 127 shifts");
            visited[state] = true;
            generator.clock(0.5f, 0.0f);
        }

        expect(generator.state() == initial,
               "127 recurrence must repeat its initial state on shift 127");
    }
}

void testDacHasEightExactMonotonicLevels() {
    float previous = burl::PatternGenerator::dacLevel(0u);
    expectNear(previous, -5.0f, 0.0f, "DAC code zero must be exactly -5 V");

    for (unsigned int code = 1u; code < 8u; ++code) {
        const float current = burl::PatternGenerator::dacLevel(code);
        expect(current > previous, "DAC levels must be strictly monotonic and unique");
        previous = current;
    }

    expectNear(previous, 5.0f, 0.0f, "DAC code seven must be exactly +5 V");

    for (unsigned int state = 0u; state < 8u; ++state) {
        burl::PatternGenerator generator(static_cast<uint8_t>(state));
        expectNear(generator.dac(2u, 1u, 0u),
                   burl::PatternGenerator::dacLevel(state), 0.0f,
                   "selected taps must carry the documented DAC weights");
    }
}

void testResetIsDeterministicAndPrevents127ZeroLock() {
    burl::PatternGenerator generator(0xa5u);
    generator.clock(1.0f, 0.0f);
    generator.reset();
    expect(generator.state() == 0xa5u,
           "reset must restore the configured seed in 8/16 mode");

    generator.setSeed(0u);
    generator.setMode(burl::PatternGenerator::Maximal127);
    generator.reset();
    expect(generator.state() == 1u,
           "reset must map a zero seed to a valid non-zero 127 state");
    for (int shift = 0; shift < 512; ++shift) {
        generator.clock(0.0f, 0.0f);
        expect(generator.state() != 0u,
               "127 mode must remain outside the zero lock state");
    }
}

void testIntermediateChangeIsDeterministic() {
    burl::PatternGenerator first(0x5au);
    burl::PatternGenerator second(0x5au);
    for (int shift = 0; shift < 1000; ++shift) {
        const float signal = static_cast<float>((shift * 37) % 201 - 100) / 100.0f;
        const float change = static_cast<float>((shift * 17) % 101) / 100.0f;
        first.clock(change, signal);
        second.clock(change, signal);
        expect(first.state() == second.state(),
               "identical shift-register inputs must produce identical states");
    }
}

} // namespace

int main() {
    testDirectRecirculationReturnsAfterEightShifts();
    testComplementedRecirculationHasSixteenShiftCycle();
    testMaximalRecurrenceVisits127NonZeroStates();
    testDacHasEightExactMonotonicLevels();
    testResetIsDeterministicAndPrevents127ZeroLock();
    testIntermediateChangeIsDeterministic();

    if (failures != 0) {
        std::cerr << failures << " test assertion(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "All pattern-generator tests passed\n";
    return EXIT_SUCCESS;
}
