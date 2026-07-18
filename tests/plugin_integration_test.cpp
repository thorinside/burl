// Copyright (c) 2026 Burl contributors
// SPDX-License-Identifier: MIT

#include <distingnt/api.h>

#include <assert.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

namespace {

int gDrawCalls = 0;
bool gDrewBurl = false;

} // namespace

extern "C" {

const _NT_globals NT_globals = { 48000, 64, nullptr, 0, 0, 0 };

void NT_drawText(int, int, const char* text, int, _NT_textAlignment,
                 _NT_textSize) {
    ++gDrawCalls;
    if (std::strcmp(text, "Burl") == 0)
        gDrewBurl = true;
}

int NT_floatToString(char* buffer, float value, int decimalPlaces) {
    return std::snprintf(buffer, kNT_parameterStringSize, "%.*f",
                         decimalPlaces, static_cast<double>(value));
}

} // extern "C"

namespace {

int findParameter(const _NT_algorithm* algorithm, uint32_t count,
                  const char* name) {
    for (uint32_t index = 0; index < count; ++index) {
        if (std::strcmp(algorithm->parameters[index].name, name) == 0)
            return static_cast<int>(index);
    }
    return -1;
}

void reseed(const _NT_factory* factory, _NT_algorithm* algorithm,
            std::vector<int16_t>& values, int parameter) {
    values[parameter] = 0;
    factory->parameterChanged(algorithm, parameter);
    values[parameter] = 1;
    factory->parameterChanged(algorithm, parameter);
    values[parameter] = 0;
    factory->parameterChanged(algorithm, parameter);
}

void testPluginIntegration() {
    assert(pluginEntry(kNT_selector_version, 0) == kNT_apiVersion13);
    assert(pluginEntry(kNT_selector_numFactories, 0) == 1);
    assert(pluginEntry(kNT_selector_factoryInfo, 1) == 0);

    const _NT_factory* factory = reinterpret_cast<const _NT_factory*>(
        pluginEntry(kNT_selector_factoryInfo, 0));
    assert(factory != nullptr);
    assert(factory->guid == NT_MULTICHAR('T', 'h', 'B', 'u'));
    assert(std::strcmp(factory->name, "Burl") == 0);
    assert((factory->tags & kNT_tagInstrument) != 0u);
    assert((factory->tags & kNT_tagFilterEQ) != 0u);
    assert(factory->draw != nullptr);
    assert(factory->customUi == nullptr);
    assert(factory->parameterString != nullptr);

    _NT_algorithmRequirements requirements = {};
    factory->calculateRequirements(requirements, nullptr);
    assert(requirements.numParameters == 50u);
    assert(requirements.sram > 0u);
    assert(requirements.dram == 0u);
    assert(requirements.dtc > 0u);

    void* sram = std::calloc(1, requirements.sram);
    void* dtc = std::calloc(1, requirements.dtc);
    assert(sram != nullptr);
    assert(dtc != nullptr);
    const _NT_algorithmMemoryPtrs pointers = {
        static_cast<uint8_t*>(sram), nullptr, static_cast<uint8_t*>(dtc), nullptr
    };
    _NT_algorithm* algorithm = factory->construct(
        pointers, requirements, nullptr);
    assert(algorithm != nullptr);
    assert(algorithm->parameters != nullptr);
    assert(algorithm->parameterPages != nullptr);
    assert(algorithm->parameterPages->numPages == 11u);

    std::vector<int16_t> values(requirements.numParameters);
    for (uint32_t index = 0; index < requirements.numParameters; ++index)
        values[index] = algorithm->parameters[index].def;
    algorithm->v = values.data();
    algorithm->vIncludingCommon = values.data();
    for (uint32_t index = 0; index < requirements.numParameters; ++index)
        factory->parameterChanged(algorithm, static_cast<int>(index));

    const char* inputNames[] = {
        "Osc 1 CV input", "Osc 2 CV input", "Filter CV input",
        "External audio input", "Clock input", "Change CV input",
        "Resonance CV input", "Mix CV input", "Reset input"
    };
    for (size_t index = 0; index < sizeof(inputNames) / sizeof(inputNames[0]);
         ++index) {
        const int parameter = findParameter(
            algorithm, requirements.numParameters, inputNames[index]);
        assert(parameter >= 0);
        assert(algorithm->parameters[parameter].min == 0);
        assert(algorithm->parameters[parameter].max == kNT_lastBus);
        assert(algorithm->parameters[parameter].def == 0);
    }

    const char* outputNames[] = {
        "Low-pass output", "Band-pass output", "High-pass output",
        "Stepped CV output", "PWM output", "XOR output",
        "Aux A output", "Aux B output"
    };
    for (size_t index = 0;
         index < sizeof(outputNames) / sizeof(outputNames[0]); ++index) {
        const int parameter = findParameter(
            algorithm, requirements.numParameters, outputNames[index]);
        assert(parameter >= 0);
        assert(algorithm->parameters[parameter].min == 1);
        assert(algorithm->parameters[parameter].max == kNT_lastBus);
        assert(algorithm->parameters[parameter].def
               == static_cast<int16_t>(13 + index));
        values[parameter + 1] = 1; // Replace output mode for deterministic checks.
    }

    const int numFrames = 8;
    std::vector<float> buses(kNT_lastBus * numFrames, 0.0f);
    factory->step(algorithm, buses.data(), numFrames / 4);
    bool anyNonZero = false;
    for (int bus = 12; bus < 20; ++bus) {
        for (int frame = 0; frame < numFrames; ++frame) {
            const float value = buses[bus * numFrames + frame];
            assert(std::isfinite(value));
            assert(value >= -10.0f && value <= 10.0f);
            anyNonZero = anyNonZero || value != 0.0f;
        }
    }
    assert(anyNonZero);

    // Firmware block sizes are expressed in groups of four frames. Exercise
    // the supported range without assuming the default 24-frame setting.
    const int blockSizes[] = { 4, 8, 12, 16, 24, 32, 48, 64, 96, 128 };
    for (size_t blockIndex = 0;
         blockIndex < sizeof(blockSizes) / sizeof(blockSizes[0]);
         ++blockIndex) {
        const int blockFrames = blockSizes[blockIndex];
        std::vector<float> blockBuses(
            kNT_lastBus * static_cast<size_t>(blockFrames), 0.0f);
        factory->step(algorithm, blockBuses.data(), blockFrames / 4);
        for (size_t index = 0; index < blockBuses.size(); ++index)
            assert(std::isfinite(blockBuses[index]));
    }

    // Quality changes use the existing algorithm and declared storage. Each
    // mode must continue processing without reconstruction or invalid output.
    const int qualityParameter = findParameter(
        algorithm, requirements.numParameters, "Quality");
    assert(qualityParameter >= 0);
    _NT_algorithm* const originalAlgorithm = algorithm;
    for (int quality = 0; quality <= 2; ++quality) {
        values[qualityParameter] = static_cast<int16_t>(quality);
        factory->parameterChanged(algorithm, qualityParameter);
        std::fill(buses.begin(), buses.end(), 0.0f);
        factory->step(algorithm, buses.data(), numFrames / 4);
        assert(algorithm == originalAlgorithm);
        for (size_t index = 0; index < buses.size(); ++index)
            assert(std::isfinite(buses[index]));
    }
    values[qualityParameter] = 1;
    factory->parameterChanged(algorithm, qualityParameter);

    const int lowPassOutput = findParameter(
        algorithm, requirements.numParameters, "Low-pass output");
    const int reseedParameter = findParameter(
        algorithm, requirements.numParameters, "Reseed");
    assert(lowPassOutput >= 0);
    assert(reseedParameter >= 0);
    values[lowPassOutput] = kNT_lastBus;
    values[lowPassOutput + 1] = 1;
    factory->parameterChanged(algorithm, lowPassOutput);
    reseed(factory, algorithm, values, reseedParameter);
    std::fill(buses.begin(), buses.end(), 0.0f);
    factory->step(algorithm, buses.data(), numFrames / 4);
    bool highBusChanged = false;
    for (int frame = 0; frame < numFrames; ++frame)
        highBusChanged = highBusChanged
            || buses[(kNT_lastBus - 1) * numFrames + frame] != 0.0f;
    assert(highBusChanged);

    const int steppedOutput = findParameter(
        algorithm, requirements.numParameters, "Stepped CV output");
    assert(steppedOutput >= 0);
    values[steppedOutput] = 50;
    values[steppedOutput + 1] = 1;
    reseed(factory, algorithm, values, reseedParameter);
    std::fill(buses.begin(), buses.end(), 0.0f);
    factory->step(algorithm, buses.data(), 1);
    const float replaced = buses[(50 - 1) * 4];

    values[steppedOutput + 1] = 0;
    reseed(factory, algorithm, values, reseedParameter);
    std::fill(buses.begin(), buses.end(), 0.0f);
    for (int frame = 0; frame < 4; ++frame)
        buses[(50 - 1) * 4 + frame] = 2.0f;
    factory->step(algorithm, buses.data(), 1);
    assert(buses[(50 - 1) * 4] == 2.0f + replaced);

    const int oscillator1Input = findParameter(
        algorithm, requirements.numParameters, "Osc 1 CV input");
    values[oscillator1Input] = 1;
    factory->parameterChanged(algorithm, oscillator1Input);
    std::fill(buses.begin(), buses.end(), 0.0f);
    for (int frame = 0; frame < 4; ++frame)
        buses[frame] = static_cast<float>(frame) - 1.5f;
    factory->step(algorithm, buses.data(), 1);

    assert(factory->draw(algorithm) == false);
    assert(gDrawCalls >= 4);
    assert(gDrewBurl);

    const int frequencyParameter = findParameter(
        algorithm, requirements.numParameters, "Osc 1 frequency");
    char frequencyText[kNT_parameterStringSize] = {};
    assert(factory->parameterString(
        algorithm, frequencyParameter, values[frequencyParameter],
        frequencyText) > 0);
    assert(std::strstr(frequencyText, "Hz") != nullptr);

    std::free(dtc);
    std::free(sram);
}

} // namespace

int main() {
    testPluginIntegration();
    std::puts("All Burl plug-in integration tests passed");
    return EXIT_SUCCESS;
}
