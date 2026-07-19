// Copyright (c) 2026 Burl contributors
// SPDX-License-Identifier: MIT

#ifndef BURL_FILTER_HPP
#define BURL_FILTER_HPP

namespace burl {

/** Allocation-free two-pole state-variable filter used by the Burl voice. */
class StateVariableFilter {
public:
    struct Frame {
        float lowPass;
        float bandPass;
        float highPass;
    };

    StateVariableFilter();

    void reset();

    /**
     * Process one sample at the voice's current internal sample rate.
     *
     * The resonance input is normalized from zero to one. Input compensation,
     * resonant pole-frequency skew, and the optional safety limiter are part of
     * the filter character rather than host-level output conditioning.
     */
    Frame process(float input, float baseCutoffHz, float cutoffOctaves,
                  float resonance, float sampleRate, bool softLimit);

    /** Return 1/Q for the curved Q = 0.5 * 40^resonance mapping. */
    static float resonanceDamping(float resonance);

private:
    static float clamp(float value, float low, float high);
    static float finiteClamp(float value, float limit);
    static float softBound(float value, float knee, float limit);

    float integrator1_;
    float integrator2_;
    float previousBand_;
};

} // namespace burl

#endif // BURL_FILTER_HPP
