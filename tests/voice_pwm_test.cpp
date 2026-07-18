// Copyright (c) 2026 Burl contributors
// SPDX-License-Identifier: MIT

#include "burl/voice.hpp"

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

void expectNear(float actual, float expected, float tolerance,
                const char* message) {
    if (std::fabs(actual - expected) > tolerance) {
        std::cerr << "FAIL: " << message << " (expected " << expected
                  << ", got " << actual << ")\n";
        ++failures;
    }
}

void testPwmIsBipolarTriangleComparator() {
    // The binary-exact rates make the triangles cross at equality while both
    // oscillator directions remain unchanged. A pulse-direction XOR would be
    // constant through this interval; the required triangle comparator flips.
    burl::Voice voice(1024.0f, 1u);
    burl::VoiceParameters parameters;
    parameters.oscillator1Hz = 8.0f;
    parameters.oscillator2Hz = 8.0f;
    parameters.oscillator1CrossModulation = 0.0f;
    parameters.oscillator2CrossModulation = 0.0f;
    parameters.oscillator1Feedback = 0.0f;
    parameters.oscillator2Feedback = 0.0f;
    parameters.quality = burl::QualityEco;
    voice.setParameters(parameters);
    voice.reset();

    burl::VoiceStatus previousStatus = voice.status();
    float previousPwm = 0.0f;
    bool havePreviousPwm = false;
    bool sawGreater = false;
    bool sawEqual = false;
    bool sawLess = false;
    unsigned int pwmChanges = 0u;

    const burl::VoiceInputs inputs;
    for (unsigned int frame = 0u; frame < 25u; ++frame) {
        const burl::VoiceOutputs outputs = voice.process(inputs);
        const burl::VoiceStatus status = voice.status();

        expect(status.oscillator1Triangle > previousStatus.oscillator1Triangle,
               "oscillator 1 pulse direction must remain high through the crossing");
        expect(status.oscillator2Triangle < previousStatus.oscillator2Triangle,
               "oscillator 2 pulse direction must remain low through the crossing");

        const float expected = status.oscillator2Triangle
                > status.oscillator1Triangle
            ? 5.0f
            : -5.0f;
        expectNear(outputs.pwm, expected, 0.0f,
                   "PWM must equal the bipolar OSC2 > OSC1 triangle comparison");
        expect(outputs.pwm == 5.0f || outputs.pwm == -5.0f,
               "PWM must be exactly bipolar at +5 V or -5 V");

        if (status.oscillator2Triangle > status.oscillator1Triangle) {
            sawGreater = true;
        } else if (status.oscillator2Triangle
                   == status.oscillator1Triangle) {
            sawEqual = true;
        } else {
            sawLess = true;
        }

        if (havePreviousPwm && outputs.pwm != previousPwm) {
            ++pwmChanges;
        }
        previousPwm = outputs.pwm;
        havePreviousPwm = true;
        previousStatus = status;
    }

    expect(sawGreater, "test interval must cover OSC2 triangle above OSC1");
    expect(sawEqual, "test interval must cover equal oscillator triangles");
    expect(sawLess, "test interval must cover OSC2 triangle below OSC1");
    expect(pwmChanges == 1u,
           "triangle crossing must change PWM while pulse directions stay fixed");
}

} // namespace

int main() {
    testPwmIsBipolarTriangleComparator();

    if (failures != 0) {
        std::cerr << failures << " PWM comparator assertion(s) failed\n";
        return EXIT_FAILURE;
    }

    std::cout << "All PWM comparator tests passed\n";
    return EXIT_SUCCESS;
}
