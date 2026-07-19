// Copyright (c) 2026 Burl contributors
// SPDX-License-Identifier: MIT

#include <distingnt/api.h>

#include <assert.h>
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
#include <vector>

std::size_t gHeapAllocationCount = 0u;

void* operator new(std::size_t size) {
    ++gHeapAllocationCount;
    void* const memory = std::malloc(size == 0u ? 1u : size);
    if (memory == nullptr)
        throw std::bad_alloc();
    return memory;
}

void* operator new[](std::size_t size) {
    ++gHeapAllocationCount;
    void* const memory = std::malloc(size == 0u ? 1u : size);
    if (memory == nullptr)
        throw std::bad_alloc();
    return memory;
}

void operator delete(void* memory) noexcept {
    std::free(memory);
}

void operator delete[](void* memory) noexcept {
    std::free(memory);
}

namespace {

int gDrawCalls = 0;
bool gDrewBurl = false;
bool gDrewActive = false;
char gLastPattern[9] = {};

bool isPatternText(const char* text) {
    for (unsigned int index = 0u; index < 8u; ++index) {
        if (text[index] != '0' && text[index] != '1')
            return false;
    }
    return text[8] == '\0';
}

} // namespace

extern "C" {

const _NT_globals NT_globals = { 48000, 64, nullptr, 0, 0, 0 };

void NT_drawText(int, int, const char* text, int, _NT_textAlignment,
                 _NT_textSize) {
    ++gDrawCalls;
    if (std::strcmp(text, "Burl") == 0)
        gDrewBurl = true;
    if (std::strcmp(text, "Active") == 0)
        gDrewActive = true;
    if (isPatternText(text))
        std::memcpy(gLastPattern, text, sizeof(gLastPattern));
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

void assertFrozenPresetAbi(const _NT_factory* factory,
                           const _NT_algorithm* algorithm, uint32_t count) {
    static const char* const kV1ParameterNames[] = {
        "Osc 1 frequency", "Osc 2 frequency", "Change", "Cutoff",
        "Resonance", "Input mix", "Osc 1 feedback", "Osc 2 feedback",
        "Osc 1 CV amount", "Osc 2 CV amount", "Change CV amount",
        "Steps mode", "Clock rate", "DAC taps", "Seed", "Reseed",
        "Stepped to cutoff", "Filter CV amount", "Resonance CV amount",
        "Mix CV amount", "Input drive", "Output limit", "Quality",
        "Aux A source", "Aux B source", "Osc 1 CV input",
        "Osc 2 CV input", "Filter CV input", "External audio input",
        "Clock input", "Change CV input", "Resonance CV input",
        "Mix CV input", "Reset input", "Low-pass output", "Low-pass mode",
        "Band-pass output", "Band-pass mode", "High-pass output",
        "High-pass mode", "Stepped CV output", "Stepped CV mode",
        "PWM output", "PWM mode", "XOR output", "XOR mode",
        "Aux A output", "Aux A mode", "Aux B output", "Aux B mode"
    };
    assert(factory->guid == NT_MULTICHAR('T', 'h', 'B', 'u'));
    assert(count == sizeof(kV1ParameterNames) / sizeof(kV1ParameterNames[0]));
    for (uint32_t index = 0; index < count; ++index) {
        assert(std::strcmp(algorithm->parameters[index].name,
                           kV1ParameterNames[index]) == 0);
    }

    // Ordinary values and mappings belong to the host preset format. Burl
    // intentionally adds no competing custom serialization or running-state
    // payload in v1.
    assert(factory->serialise == nullptr);
    assert(factory->deserialise == nullptr);
}

_NT_algorithm* constructTestAlgorithm(
    const _NT_factory* factory, const _NT_algorithmRequirements& requirements,
    void*& sram, void*& dtc) {
    sram = std::calloc(1, requirements.sram);
    dtc = std::calloc(1, requirements.dtc);
    assert(sram != nullptr);
    assert(dtc != nullptr);
    const _NT_algorithmMemoryPtrs pointers = {
        static_cast<uint8_t*>(sram), nullptr, static_cast<uint8_t*>(dtc), nullptr
    };
    _NT_algorithm* algorithm = factory->construct(
        pointers, requirements, nullptr);
    assert(algorithm != nullptr);
    return algorithm;
}

void applyParameterSnapshot(const _NT_factory* factory,
                            _NT_algorithm* algorithm,
                            const std::vector<int16_t>& values, bool reverse) {
    algorithm->v = values.data();
    algorithm->vIncludingCommon = values.data();
    if (reverse) {
        for (size_t offset = 0; offset < values.size(); ++offset) {
            const int parameter = static_cast<int>(values.size() - 1u - offset);
            factory->parameterChanged(algorithm, parameter);
        }
    } else {
        for (size_t parameter = 0; parameter < values.size(); ++parameter)
            factory->parameterChanged(algorithm, static_cast<int>(parameter));
    }
}

std::vector<float> renderPresetFixture(const _NT_factory* factory,
                                       _NT_algorithm* algorithm) {
    const int numFrames = 128;
    std::vector<float> buses(kNT_lastBus * numFrames, 0.0f);
    for (int frame = 0; frame < numFrames; ++frame) {
        buses[0 * numFrames + frame] =
            0.25f * static_cast<float>((frame % 17) - 8);
        buses[1 * numFrames + frame] =
            0.2f * static_cast<float>((frame % 19) - 9);
        buses[2 * numFrames + frame] =
            0.1f * static_cast<float>((frame % 23) - 11);
        buses[3 * numFrames + frame] =
            0.15f * static_cast<float>((frame % 29) - 14);
        buses[4 * numFrames + frame] = (frame % 16) < 8 ? 0.0f : 5.0f;
        buses[5 * numFrames + frame] =
            0.125f * static_cast<float>((frame % 13) - 6);
        buses[6 * numFrames + frame] =
            0.1f * static_cast<float>((frame % 11) - 5);
        buses[7 * numFrames + frame] =
            0.1f * static_cast<float>((frame % 7) - 3);
        buses[8 * numFrames + frame] = 0.0f;
    }
    factory->step(algorithm, buses.data(), numFrames / 4);
    return buses;
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
    assertFrozenPresetAbi(factory, algorithm, requirements.numParameters);

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

float renderInputDriveRms(const _NT_factory* factory,
                          _NT_algorithm* algorithm,
                          std::vector<int16_t>& values,
                          int driveParameter,
                          int reseedParameter,
                          int driveValue,
                          float inputAmplitude) {
    values[driveParameter] = static_cast<int16_t>(driveValue);
    factory->parameterChanged(algorithm, driveParameter);
    reseed(factory, algorithm, values, reseedParameter);

    const int numFrames = 128;
    const unsigned int warmupBlocks = 128u;
    const unsigned int captureBlocks = 128u;
    std::vector<float> buses(kNT_lastBus * numFrames, 0.0f);
    double squared = 0.0;
    unsigned int samples = 0u;
    unsigned int absoluteFrame = 0u;
    for (unsigned int block = 0u;
         block < warmupBlocks + captureBlocks; ++block) {
        std::fill(buses.begin(), buses.end(), 0.0f);
        for (int frame = 0; frame < numFrames; ++frame, ++absoluteFrame) {
            buses[frame] = inputAmplitude * std::sin(
                2.0 * 3.14159265358979323846 * 100.0
                * static_cast<double>(absoluteFrame)
                / static_cast<double>(NT_globals.sampleRate));
        }
        factory->step(algorithm, buses.data(), numFrames / 4);
        if (block >= warmupBlocks) {
            for (int frame = 0; frame < numFrames; ++frame) {
                const float output = buses[12 * numFrames + frame];
                squared += static_cast<double>(output) * output;
                ++samples;
            }
        }
    }
    return static_cast<float>(std::sqrt(squared / samples));
}

void testInputDriveHostControlAndTransparency() {
    const _NT_factory* factory = reinterpret_cast<const _NT_factory*>(
        pluginEntry(kNT_selector_factoryInfo, 0));
    assert(factory != nullptr);

    _NT_algorithmRequirements requirements = {};
    factory->calculateRequirements(requirements, nullptr);
    void* sram = nullptr;
    void* dtc = nullptr;
    _NT_algorithm* algorithm = constructTestAlgorithm(
        factory, requirements, sram, dtc);

    std::vector<int16_t> values(requirements.numParameters);
    for (uint32_t index = 0; index < requirements.numParameters; ++index)
        values[index] = algorithm->parameters[index].def;

    const int drive = findParameter(
        algorithm, requirements.numParameters, "Input drive");
    const int inputMix = findParameter(
        algorithm, requirements.numParameters, "Input mix");
    const int externalAudio = findParameter(
        algorithm, requirements.numParameters, "External audio input");
    const int cutoff = findParameter(
        algorithm, requirements.numParameters, "Cutoff");
    const int resonance = findParameter(
        algorithm, requirements.numParameters, "Resonance");
    const int steppedToCutoff = findParameter(
        algorithm, requirements.numParameters, "Stepped to cutoff");
    const int outputLimit = findParameter(
        algorithm, requirements.numParameters, "Output limit");
    const int quality = findParameter(
        algorithm, requirements.numParameters, "Quality");
    const int lowPassOutput = findParameter(
        algorithm, requirements.numParameters, "Low-pass output");
    const int reseedParameter = findParameter(
        algorithm, requirements.numParameters, "Reseed");
    assert(drive >= 0 && inputMix >= 0 && externalAudio >= 0);
    assert(cutoff >= 0 && resonance >= 0 && steppedToCutoff >= 0);
    assert(outputLimit >= 0 && quality >= 0 && lowPassOutput >= 0);
    assert(reseedParameter >= 0);

    assert(algorithm->parameters[drive].min == 25);
    assert(algorithm->parameters[drive].max == 400);
    assert(algorithm->parameters[drive].def == 100);
    char driveText[kNT_parameterStringSize] = {};
    assert(factory->parameterString(
        algorithm, drive, 25, driveText) > 0);
    assert(std::strcmp(driveText, "0.25x") == 0);
    assert(factory->parameterString(
        algorithm, drive, 400, driveText) > 0);
    assert(std::strcmp(driveText, "4.00x") == 0);

    values[inputMix] = 0;          // Fully external.
    values[externalAudio] = 1;     // Bus 1.
    values[cutoff] = 10000;        // Keep the 100 Hz measurement in-band.
    values[resonance] = 0;
    values[steppedToCutoff] = 0;
    values[outputLimit] = 0;
    values[quality] = 0;
    values[lowPassOutput] = 13;
    values[lowPassOutput + 1] = 1;
    applyParameterSnapshot(factory, algorithm, values, false);

    const float lowQuarter = renderInputDriveRms(
        factory, algorithm, values, drive, reseedParameter, 25, 0.25f);
    const float lowHalf = renderInputDriveRms(
        factory, algorithm, values, drive, reseedParameter, 50, 0.25f);
    const float lowOne = renderInputDriveRms(
        factory, algorithm, values, drive, reseedParameter, 100, 0.25f);
    const float lowTwo = renderInputDriveRms(
        factory, algorithm, values, drive, reseedParameter, 200, 0.25f);
    assert(std::fabs(lowHalf / lowQuarter - 2.0f) < 0.005f);
    assert(std::fabs(lowOne / lowHalf - 2.0f) < 0.005f);
    assert(std::fabs(lowTwo / lowOne - 2.0f) < 0.005f);

    const float normalOne = renderInputDriveRms(
        factory, algorithm, values, drive, reseedParameter, 100, 5.0f);
    const float normalTwo = renderInputDriveRms(
        factory, algorithm, values, drive, reseedParameter, 200, 5.0f);
    const float normalFour = renderInputDriveRms(
        factory, algorithm, values, drive, reseedParameter, 400, 5.0f);
    assert(std::fabs(normalTwo / normalOne - 2.0f) < 0.005f);
    assert(std::fabs(normalFour / normalTwo - 2.0f) < 0.005f);

    std::free(dtc);
    std::free(sram);
}

const std::size_t kMemoryGuardBytes = 64u;
const unsigned char kMemoryGuardValue = 0xa5u;

class GuardedMemory {
public:
    explicit GuardedMemory(std::size_t payloadBytes)
        : payloadBytes_(payloadBytes),
          storage_(payloadBytes + 2u * kMemoryGuardBytes, kMemoryGuardValue) {
        std::fill(storage_.begin() + kMemoryGuardBytes,
                  storage_.begin() + kMemoryGuardBytes + payloadBytes_, 0u);
    }

    uint8_t* data() {
        return storage_.data() + kMemoryGuardBytes;
    }

    bool guardsIntact() const {
        for (std::size_t index = 0u; index < kMemoryGuardBytes; ++index) {
            if (storage_[index] != kMemoryGuardValue
                || storage_[kMemoryGuardBytes + payloadBytes_ + index]
                    != kMemoryGuardValue) {
                return false;
            }
        }
        return true;
    }

private:
    std::size_t payloadBytes_;
    std::vector<unsigned char> storage_;
};

void assertActiveOutputBlock(const std::vector<float>& buses, int numFrames) {
    for (int frame = 0; frame < numFrames; ++frame) {
        bool active = false;
        for (int bus = 12; bus < 20; ++bus) {
            const float value = buses[bus * numFrames + frame];
            assert(std::isfinite(value));
            assert(value >= -10.0f && value <= 10.0f);
            active = active || value != 0.0f;
        }
        assert(active);
    }
}

void testRuntimeQualitySwitching() {
    const _NT_factory* factory = reinterpret_cast<const _NT_factory*>(
        pluginEntry(kNT_selector_factoryInfo, 0));
    assert(factory != nullptr);

    _NT_algorithmRequirements requirements = {};
    factory->calculateRequirements(requirements, nullptr);
    assert(requirements.sram > 0u);
    assert(requirements.dtc > 0u);
    assert(requirements.dram == 0u);
    assert(requirements.itc == 0u);

    GuardedMemory sram(requirements.sram);
    GuardedMemory dtc(requirements.dtc);
    const _NT_algorithmMemoryPtrs pointers = {
        sram.data(), nullptr, dtc.data(), nullptr
    };
    _NT_algorithm* const algorithm = factory->construct(
        pointers, requirements, nullptr);
    assert(algorithm != nullptr);

    std::vector<int16_t> values(requirements.numParameters);
    for (uint32_t index = 0; index < requirements.numParameters; ++index)
        values[index] = algorithm->parameters[index].def;
    algorithm->v = values.data();
    algorithm->vIncludingCommon = values.data();

    const char* const outputNames[] = {
        "Low-pass output", "Band-pass output", "High-pass output",
        "Stepped CV output", "PWM output", "XOR output",
        "Aux A output", "Aux B output"
    };
    for (std::size_t index = 0u;
         index < sizeof(outputNames) / sizeof(outputNames[0]); ++index) {
        const int output = findParameter(
            algorithm, requirements.numParameters, outputNames[index]);
        assert(output >= 0);
        values[output + 1] = 1;
    }
    for (uint32_t index = 0; index < requirements.numParameters; ++index)
        factory->parameterChanged(algorithm, static_cast<int>(index));

    const int qualityParameter = findParameter(
        algorithm, requirements.numParameters, "Quality");
    assert(qualityParameter >= 0);
    const _NT_parameter* const originalParameters = algorithm->parameters;
    const _NT_parameterPages* const originalPages = algorithm->parameterPages;
    const int16_t* const originalValues = algorithm->v;
    const int16_t* const originalCommonValues = algorithm->vIncludingCommon;

    const int numFrames = static_cast<int>(NT_globals.maxFramesPerStep);
    assert(numFrames > 0 && (numFrames % 4) == 0);
    std::vector<float> buses(
        static_cast<std::size_t>(kNT_lastBus) * numFrames, 0.0f);

    // Establish non-reset oscillator, filter, pattern, clock, and display state.
    for (unsigned int block = 0u; block < 32u; ++block) {
        std::fill(buses.begin(), buses.end(), 0.0f);
        factory->step(algorithm, buses.data(), numFrames / 4);
    }
    assertActiveOutputBlock(buses, numFrames);

    const int qualitySequence[] = {0, 2, 1, 2, 0, 1};
    const unsigned int switchBlocks = 192u;
    for (unsigned int block = 0u; block < switchBlocks; ++block) {
        gDrewActive = false;
        factory->draw(algorithm);
        assert(gDrewActive);
        char patternBefore[sizeof(gLastPattern)];
        std::memcpy(patternBefore, gLastPattern, sizeof(patternBefore));

        values[qualityParameter] = static_cast<int16_t>(
            qualitySequence[block
                % (sizeof(qualitySequence) / sizeof(qualitySequence[0]))]);
        const std::size_t allocationsBeforeChange = gHeapAllocationCount;
        factory->parameterChanged(algorithm, qualityParameter);
        assert(gHeapAllocationCount == allocationsBeforeChange);

        // A quality callback must preserve the active running state and every
        // host-owned pointer; only the cached quality factor may change.
        assert(algorithm == reinterpret_cast<_NT_algorithm*>(sram.data()));
        assert(algorithm->parameters == originalParameters);
        assert(algorithm->parameterPages == originalPages);
        assert(algorithm->v == originalValues);
        assert(algorithm->vIncludingCommon == originalCommonValues);
        gDrewActive = false;
        std::memset(gLastPattern, 0, sizeof(gLastPattern));
        factory->draw(algorithm);
        assert(gDrewActive);
        assert(std::memcmp(patternBefore, gLastPattern,
                           sizeof(patternBefore)) == 0);
        assert(sram.guardsIntact());
        assert(dtc.guardsIntact());

        std::fill(buses.begin(), buses.end(), 0.0f);
        const std::size_t allocationsBeforeStep = gHeapAllocationCount;
        factory->step(algorithm, buses.data(), numFrames / 4);
        assert(gHeapAllocationCount == allocationsBeforeStep);
        assertActiveOutputBlock(buses, numFrames);
        assert(sram.guardsIntact());
        assert(dtc.guardsIntact());
    }

    _NT_algorithmRequirements afterSwitching = {};
    factory->calculateRequirements(afterSwitching, nullptr);
    assert(afterSwitching.numParameters == requirements.numParameters);
    assert(afterSwitching.sram == requirements.sram);
    assert(afterSwitching.dram == requirements.dram);
    assert(afterSwitching.dtc == requirements.dtc);
    assert(afterSwitching.itc == requirements.itc);
}

void testPresetCompatibility() {
    const _NT_factory* factory = reinterpret_cast<const _NT_factory*>(
        pluginEntry(kNT_selector_factoryInfo, 0));
    assert(factory != nullptr);

    _NT_algorithmRequirements requirements = {};
    factory->calculateRequirements(requirements, nullptr);
    static const int16_t kPresetValues[] = {
        8123, 4210, -237, 6920, 777, 340,
        -612, 731, -456, 567,
        321, 1, 1, 1, 113, 0,
        -528, 643, -375, 284, 275, 0,
        2, 8, 9,
        1, 2, 3, 4, 5, 6, 7, 8, 9,
        41, 1, 42, 1, 43, 1, 44, 1,
        45, 1, 46, 1, 47, 1, 48, 1
    };
    assert(requirements.numParameters
           == sizeof(kPresetValues) / sizeof(kPresetValues[0]));

    std::vector<int16_t> savedValues(
        kPresetValues,
        kPresetValues + sizeof(kPresetValues) / sizeof(kPresetValues[0]));

    void* reloadedSram = nullptr;
    void* reloadedDtc = nullptr;
    _NT_algorithm* reloaded = constructTestAlgorithm(
        factory, requirements, reloadedSram, reloadedDtc);
    assertFrozenPresetAbi(factory, reloaded, requirements.numParameters);
    for (uint32_t index = 0; index < requirements.numParameters; ++index) {
        assert(savedValues[index] >= reloaded->parameters[index].min);
        assert(savedValues[index] <= reloaded->parameters[index].max);
    }

    std::vector<int16_t> activeValues(savedValues);
    applyParameterSnapshot(factory, reloaded, activeValues, false);
    const std::vector<float> initialSeededRender =
        renderPresetFixture(factory, reloaded);

    // Move every determinism-relevant DSP subsystem away from the seeded
    // state, then replace the host-owned parameter array with defaults.
    renderPresetFixture(factory, reloaded);
    for (uint32_t index = 0; index < requirements.numParameters; ++index)
        activeValues[index] = reloaded->parameters[index].def;
    applyParameterSnapshot(factory, reloaded, activeValues, false);
    renderPresetFixture(factory, reloaded);

    // A host preset load makes the complete positional value array visible
    // before callbacks. Reverse callback order stresses that Seed restoration
    // does not depend on callbacks arriving in declaration order.
    activeValues = savedValues;
    applyParameterSnapshot(factory, reloaded, activeValues, true);
    assert(activeValues == savedValues);
    const std::vector<float> reloadedSeededRender =
        renderPresetFixture(factory, reloaded);
    assert(initialSeededRender.size() == reloadedSeededRender.size());
    assert(std::memcmp(initialSeededRender.data(), reloadedSeededRender.data(),
                       initialSeededRender.size() * sizeof(float)) == 0);

    // A newly constructed instance located through the same frozen GUID and
    // positional ABI must produce the same seeded render.
    void* freshSram = nullptr;
    void* freshDtc = nullptr;
    _NT_algorithm* fresh = constructTestAlgorithm(
        factory, requirements, freshSram, freshDtc);
    assertFrozenPresetAbi(factory, fresh, requirements.numParameters);
    std::vector<int16_t> freshValues(savedValues);
    applyParameterSnapshot(factory, fresh, freshValues, false);
    const std::vector<float> freshSeededRender =
        renderPresetFixture(factory, fresh);
    assert(std::memcmp(initialSeededRender.data(), freshSeededRender.data(),
                       initialSeededRender.size() * sizeof(float)) == 0);

    std::free(freshDtc);
    std::free(freshSram);
    std::free(reloadedDtc);
    std::free(reloadedSram);
}

} // namespace

int main() {
    testPluginIntegration();
    testInputDriveHostControlAndTransparency();
    testRuntimeQualitySwitching();
    testPresetCompatibility();
    std::puts("All Burl plug-in integration tests passed");
    return EXIT_SUCCESS;
}
