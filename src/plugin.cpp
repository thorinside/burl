// Copyright (c) 2026 Burl contributors
// SPDX-License-Identifier: MIT

#include <distingnt/api.h>

#include "burl/voice.hpp"

#include <cmath>
#include <new>
#include <stddef.h>
#include <stdint.h>

namespace {

template <typename T, size_t N>
constexpr size_t arrayCount(const T (&)[N]) {
    return N;
}

constexpr _NT_parameter makeParameter(const char* name, int16_t minimum,
                                      int16_t maximum, int16_t defaultValue,
                                      uint8_t unit, uint8_t scaling,
                                      char const * const * enumStrings) {
    return { name, minimum, maximum, defaultValue, unit, scaling, enumStrings };
}

constexpr _NT_parameterPage makePage(const char* name, uint8_t count,
                                     const uint8_t* parameters) {
    return { name, count, 0, { 0, 0 }, parameters };
}

// Presets identify the factory by GUID and store these parameters positionally.
// This order is the Burl v1 preset ABI: do not reorder or rename entries.
enum Parameter {
    kParamOscillator1Frequency,
    kParamOscillator2Frequency,
    kParamChange,
    kParamCutoff,
    kParamResonance,
    kParamInputMix,

    kParamOscillator1Feedback,
    kParamOscillator2Feedback,
    kParamOscillator1CvAmount,
    kParamOscillator2CvAmount,

    kParamChangeCvAmount,
    kParamStepsMode,
    kParamClockRate,
    kParamDacTaps,
    kParamSeed,
    kParamReseed,

    kParamSteppedToCutoff,
    kParamFilterCvAmount,
    kParamResonanceCvAmount,
    kParamMixCvAmount,
    kParamInputDrive,
    kParamOutputLimit,

    kParamQuality,
    kParamAuxASource,
    kParamAuxBSource,

    kParamOscillator1CvInput,
    kParamOscillator2CvInput,
    kParamFilterCvInput,
    kParamExternalAudioInput,
    kParamClockInput,
    kParamChangeCvInput,
    kParamResonanceCvInput,
    kParamMixCvInput,
    kParamResetInput,

    kParamLowPassOutput,
    kParamLowPassOutputMode,
    kParamBandPassOutput,
    kParamBandPassOutputMode,
    kParamHighPassOutput,
    kParamHighPassOutputMode,
    kParamSteppedCvOutput,
    kParamSteppedCvOutputMode,
    kParamPwmOutput,
    kParamPwmOutputMode,
    kParamRegisterXorOutput,
    kParamRegisterXorOutputMode,
    kParamAuxAOutput,
    kParamAuxAOutputMode,
    kParamAuxBOutput,
    kParamAuxBOutputMode,

    kNumParameters
};

static char const * const kStepsModeStrings[] = { "8/16", "127" };
static char const * const kClockRateStrings[] = { "Single", "Double" };
static char const * const kDacTapStrings[] = { "6,7,8", "2,4,7" };
static char const * const kOutputLimitStrings[] = { "Off", "Soft" };
static char const * const kQualityStrings[] = { "Eco", "Normal", "High" };
static char const * const kAuxSourceStrings[] = {
    "Osc 1 triangle", "Osc 1 pulse", "Osc 2 triangle", "Osc 2 pulse",
    "PWM", "XOR", "Stepped CV", "Low-pass", "Band-pass", "High-pass"
};

static const _NT_parameter kParameters[] = {
    makeParameter("Osc 1 frequency", 0, 10000, 6178, kNT_unitHasStrings,
                  kNT_scalingNone, nullptr),
    makeParameter("Osc 2 frequency", 0, 10000, 5989, kNT_unitHasStrings,
                  kNT_scalingNone, nullptr),
    makeParameter("Change", -1000, 1000, 0, kNT_unitNone,
                  kNT_scaling1000, nullptr),
    makeParameter("Cutoff", 0, 10000, 4986, kNT_unitHasStrings,
                  kNT_scalingNone, nullptr),
    makeParameter("Resonance", 0, 1000, 620, kNT_unitPercent,
                  kNT_scaling10, nullptr),
    makeParameter("Input mix", 0, 1000, 1000, kNT_unitPercent,
                  kNT_scaling10, nullptr),

    makeParameter("Osc 1 feedback", -1000, 1000, 350, kNT_unitNone,
                  kNT_scaling1000, nullptr),
    makeParameter("Osc 2 feedback", -1000, 1000, 450, kNT_unitNone,
                  kNT_scaling1000, nullptr),
    makeParameter("Osc 1 CV amount", -1000, 1000, 150, kNT_unitNone,
                  kNT_scaling1000, nullptr),
    makeParameter("Osc 2 CV amount", -1000, 1000, 150, kNT_unitNone,
                  kNT_scaling1000, nullptr),

    makeParameter("Change CV amount", -1000, 1000, 0, kNT_unitNone,
                  kNT_scaling1000, nullptr),
    makeParameter("Steps mode", 0, 1, 0, kNT_unitEnum, kNT_scalingNone,
                  kStepsModeStrings),
    makeParameter("Clock rate", 0, 1, 0, kNT_unitEnum, kNT_scalingNone,
                  kClockRateStrings),
    makeParameter("DAC taps", 0, 1, 0, kNT_unitEnum, kNT_scalingNone,
                  kDacTapStrings),
    makeParameter("Seed", 1, 255, 0x5d, kNT_unitNone, kNT_scalingNone,
                  nullptr),
    makeParameter("Reseed", 0, 1, 0, kNT_unitConfirm, kNT_scalingNone,
                  nullptr),

    makeParameter("Stepped to cutoff", -1000, 1000, 350, kNT_unitNone,
                  kNT_scaling1000, nullptr),
    makeParameter("Filter CV amount", -1000, 1000, 0, kNT_unitNone,
                  kNT_scaling1000, nullptr),
    makeParameter("Resonance CV amount", -1000, 1000, 0, kNT_unitNone,
                  kNT_scaling1000, nullptr),
    makeParameter("Mix CV amount", -1000, 1000, 0, kNT_unitNone,
                  kNT_scaling1000, nullptr),
    makeParameter("Input drive", 25, 400, 100, kNT_unitHasStrings,
                  kNT_scaling100, nullptr),
    makeParameter("Output limit", 0, 1, 1, kNT_unitEnum, kNT_scalingNone,
                  kOutputLimitStrings),

    makeParameter("Quality", 0, 2, 1, kNT_unitEnum, kNT_scalingNone,
                  kQualityStrings),
    makeParameter("Aux A source", 0, 9, 0, kNT_unitEnum, kNT_scalingNone,
                  kAuxSourceStrings),
    makeParameter("Aux B source", 0, 9, 2, kNT_unitEnum, kNT_scalingNone,
                  kAuxSourceStrings),

    makeParameter("Osc 1 CV input", 0, kNT_lastBus, 0, kNT_unitCvInput,
                  kNT_scalingNone, nullptr),
    makeParameter("Osc 2 CV input", 0, kNT_lastBus, 0, kNT_unitCvInput,
                  kNT_scalingNone, nullptr),
    makeParameter("Filter CV input", 0, kNT_lastBus, 0, kNT_unitCvInput,
                  kNT_scalingNone, nullptr),
    makeParameter("External audio input", 0, kNT_lastBus, 0,
                  kNT_unitAudioInput, kNT_scalingNone, nullptr),
    makeParameter("Clock input", 0, kNT_lastBus, 0, kNT_unitCvInput,
                  kNT_scalingNone, nullptr),
    makeParameter("Change CV input", 0, kNT_lastBus, 0, kNT_unitCvInput,
                  kNT_scalingNone, nullptr),
    makeParameter("Resonance CV input", 0, kNT_lastBus, 0, kNT_unitCvInput,
                  kNT_scalingNone, nullptr),
    makeParameter("Mix CV input", 0, kNT_lastBus, 0, kNT_unitCvInput,
                  kNT_scalingNone, nullptr),
    makeParameter("Reset input", 0, kNT_lastBus, 0, kNT_unitCvInput,
                  kNT_scalingNone, nullptr),

    makeParameter("Low-pass output", 1, kNT_lastBus, 13,
                  kNT_unitAudioOutput, kNT_scalingNone, nullptr),
    makeParameter("Low-pass mode", 0, 1, 0, kNT_unitOutputMode,
                  kNT_scalingNone, nullptr),
    makeParameter("Band-pass output", 1, kNT_lastBus, 14,
                  kNT_unitAudioOutput, kNT_scalingNone, nullptr),
    makeParameter("Band-pass mode", 0, 1, 0, kNT_unitOutputMode,
                  kNT_scalingNone, nullptr),
    makeParameter("High-pass output", 1, kNT_lastBus, 15,
                  kNT_unitAudioOutput, kNT_scalingNone, nullptr),
    makeParameter("High-pass mode", 0, 1, 0, kNT_unitOutputMode,
                  kNT_scalingNone, nullptr),
    makeParameter("Stepped CV output", 1, kNT_lastBus, 16,
                  kNT_unitCvOutput, kNT_scalingNone, nullptr),
    makeParameter("Stepped CV mode", 0, 1, 0, kNT_unitOutputMode,
                  kNT_scalingNone, nullptr),
    makeParameter("PWM output", 1, kNT_lastBus, 17, kNT_unitCvOutput,
                  kNT_scalingNone, nullptr),
    makeParameter("PWM mode", 0, 1, 0, kNT_unitOutputMode,
                  kNT_scalingNone, nullptr),
    makeParameter("XOR output", 1, kNT_lastBus, 18, kNT_unitCvOutput,
                  kNT_scalingNone, nullptr),
    makeParameter("XOR mode", 0, 1, 0, kNT_unitOutputMode,
                  kNT_scalingNone, nullptr),
    makeParameter("Aux A output", 1, kNT_lastBus, 19, kNT_unitCvOutput,
                  kNT_scalingNone, nullptr),
    makeParameter("Aux A mode", 0, 1, 0, kNT_unitOutputMode,
                  kNT_scalingNone, nullptr),
    makeParameter("Aux B output", 1, kNT_lastBus, 20, kNT_unitCvOutput,
                  kNT_scalingNone, nullptr),
    makeParameter("Aux B mode", 0, 1, 0, kNT_unitOutputMode,
                  kNT_scalingNone, nullptr),
};

static_assert(arrayCount(kParameters) == kNumParameters,
              "Burl parameter table mismatch");
static_assert(kNumParameters == 50 && kParamSeed == 14
                  && kParamOscillator1CvInput == 25
                  && kParamLowPassOutput == 34
                  && kParamAuxBOutputMode == 49,
              "Burl v1 positional preset ABI changed");

static const uint8_t kMainPage[] = {
    kParamOscillator1Frequency, kParamOscillator2Frequency, kParamChange,
    kParamCutoff, kParamResonance, kParamInputMix
};
static const uint8_t kFeedbackPage[] = {
    kParamOscillator1Feedback, kParamOscillator2Feedback,
    kParamOscillator1CvAmount, kParamOscillator2CvAmount
};
static const uint8_t kPatternPage[] = {
    kParamChangeCvAmount, kParamStepsMode, kParamClockRate, kParamDacTaps,
    kParamSeed, kParamReseed
};
static const uint8_t kFilterPage[] = {
    kParamSteppedToCutoff, kParamFilterCvAmount, kParamResonanceCvAmount,
    kParamMixCvAmount, kParamInputDrive, kParamOutputLimit
};
static const uint8_t kQualityPage[] = {
    kParamQuality, kParamAuxASource, kParamAuxBSource
};
static const uint8_t kInputsOnePage[] = {
    kParamOscillator1CvInput, kParamOscillator2CvInput, kParamFilterCvInput,
    kParamExternalAudioInput, kParamClockInput
};
static const uint8_t kInputsTwoPage[] = {
    kParamChangeCvInput, kParamResonanceCvInput, kParamMixCvInput,
    kParamResetInput
};
static const uint8_t kOutputsOnePage[] = {
    kParamLowPassOutput, kParamLowPassOutputMode,
    kParamBandPassOutput, kParamBandPassOutputMode
};
static const uint8_t kOutputsTwoPage[] = {
    kParamHighPassOutput, kParamHighPassOutputMode,
    kParamSteppedCvOutput, kParamSteppedCvOutputMode
};
static const uint8_t kOutputsThreePage[] = {
    kParamPwmOutput, kParamPwmOutputMode,
    kParamRegisterXorOutput, kParamRegisterXorOutputMode
};
static const uint8_t kOutputsFourPage[] = {
    kParamAuxAOutput, kParamAuxAOutputMode,
    kParamAuxBOutput, kParamAuxBOutputMode
};

static const _NT_parameterPage kPages[] = {
    makePage("Main", static_cast<uint8_t>(arrayCount(kMainPage)), kMainPage),
    makePage("Feedback", static_cast<uint8_t>(arrayCount(kFeedbackPage)),
             kFeedbackPage),
    makePage("Pattern", static_cast<uint8_t>(arrayCount(kPatternPage)),
             kPatternPage),
    makePage("Filter", static_cast<uint8_t>(arrayCount(kFilterPage)),
             kFilterPage),
    makePage("Quality", static_cast<uint8_t>(arrayCount(kQualityPage)),
             kQualityPage),
    makePage("Inputs 1", static_cast<uint8_t>(arrayCount(kInputsOnePage)),
             kInputsOnePage),
    makePage("Inputs 2", static_cast<uint8_t>(arrayCount(kInputsTwoPage)),
             kInputsTwoPage),
    makePage("Outputs 1", static_cast<uint8_t>(arrayCount(kOutputsOnePage)),
             kOutputsOnePage),
    makePage("Outputs 2", static_cast<uint8_t>(arrayCount(kOutputsTwoPage)),
             kOutputsTwoPage),
    makePage("Outputs 3", static_cast<uint8_t>(arrayCount(kOutputsThreePage)),
             kOutputsThreePage),
    makePage("Outputs 4", static_cast<uint8_t>(arrayCount(kOutputsFourPage)),
             kOutputsFourPage),
};

static const _NT_parameterPages kParameterPages = {
    static_cast<uint32_t>(arrayCount(kPages)), kPages
};

static const int kInputParameters[] = {
    kParamOscillator1CvInput, kParamOscillator2CvInput, kParamFilterCvInput,
    kParamExternalAudioInput, kParamClockInput, kParamChangeCvInput,
    kParamResonanceCvInput, kParamMixCvInput, kParamResetInput
};

static const int kOutputParameters[] = {
    kParamLowPassOutput, kParamBandPassOutput, kParamHighPassOutput,
    kParamSteppedCvOutput, kParamPwmOutput, kParamRegisterXorOutput,
    kParamAuxAOutput, kParamAuxBOutput
};

struct BurlAlgorithm : public _NT_algorithm {
    explicit BurlAlgorithm(burl::Voice* voiceIn)
        : voice(voiceIn), displayStatus(voiceIn->status()), lastReseed(0) {
        parameters = kParameters;
        parameterPages = &kParameterPages;
        vIncludingCommon = nullptr;
        v = nullptr;
    }

    burl::Voice* voice;
    burl::VoiceStatus displayStatus;
    int16_t lastReseed;
};

float clamp(float value, float low, float high) {
    return value < low ? low : (value > high ? high : value);
}

float frequencyFromControl(int value, float minimum, float maximum) {
    const float normalized = clamp(static_cast<float>(value) / 10000.0f,
                                   0.0f, 1.0f);
    return minimum * std::pow(maximum / minimum, normalized);
}

burl::VoiceParameters defaultVoiceParameters() {
    burl::VoiceParameters parameters;
    parameters.oscillator1Hz = 60.0f;
    parameters.oscillator2Hz = 47.0f;
    parameters.oscillator1CrossModulation = 0.15f;
    parameters.oscillator2CrossModulation = 0.15f;
    parameters.oscillator1Feedback = 0.35f;
    parameters.oscillator2Feedback = 0.45f;
    parameters.change = 0.5f;
    parameters.filterCutoffHz = 250.0f;
    parameters.filterResonance = 0.62f;
    parameters.filterFeedback = 0.35f;
    parameters.externalCutoffModulation = 0.0f;
    parameters.externalInputMix = 0.0f;
    parameters.changeCvAmount = 0.0f;
    parameters.resonanceCvAmount = 0.0f;
    parameters.mixCvAmount = 0.0f;
    parameters.inputDrive = 1.0f;
    parameters.useExternalOscillator1Cv = false;
    parameters.useExternalOscillator2Cv = false;
    parameters.useExternalClock = false;
    parameters.doubleEdgeClock = false;
    parameters.maximal127Mode = false;
    parameters.safetyLimit = true;
    parameters.quality = burl::QualityNormal;
    parameters.dacMsbTap = 7u;
    parameters.dacMiddleTap = 6u;
    parameters.dacLsbTap = 5u;
    parameters.auxASource = burl::AuxOscillator1Triangle;
    parameters.auxBSource = burl::AuxOscillator2Triangle;
    return parameters;
}

void calculateRequirements(_NT_algorithmRequirements& requirements,
                           const int32_t*) {
    requirements.numParameters = kNumParameters;
    requirements.sram = sizeof(BurlAlgorithm);
    requirements.dram = 0;
    requirements.dtc = sizeof(burl::Voice);
    requirements.itc = 0;
}

_NT_algorithm* construct(const _NT_algorithmMemoryPtrs& pointers,
                         const _NT_algorithmRequirements&, const int32_t*) {
    burl::Voice* voice = new (pointers.dtc)
        burl::Voice(static_cast<float>(NT_globals.sampleRate), 0x5du);
    voice->setParameters(defaultVoiceParameters());
    voice->reset();
    return new (pointers.sram) BurlAlgorithm(voice);
}

void applyParameters(BurlAlgorithm* algorithm) {
    if (algorithm->v == nullptr)
        return;

    burl::VoiceParameters parameters = algorithm->voice->parameters();
    parameters.oscillator1Hz = frequencyFromControl(
        algorithm->v[kParamOscillator1Frequency], 0.02f, 8500.0f);
    parameters.oscillator2Hz = frequencyFromControl(
        algorithm->v[kParamOscillator2Frequency], 0.02f, 8500.0f);
    parameters.change = clamp(
        0.5f + static_cast<float>(algorithm->v[kParamChange]) / 2000.0f,
        0.0f, 1.0f);
    parameters.filterCutoffHz = frequencyFromControl(
        algorithm->v[kParamCutoff], 4.0f, 16000.0f);
    parameters.filterResonance = clamp(
        static_cast<float>(algorithm->v[kParamResonance]) / 1000.0f,
        0.0f, 1.0f);
    parameters.externalInputMix = 1.0f - clamp(
        static_cast<float>(algorithm->v[kParamInputMix]) / 1000.0f,
        0.0f, 1.0f);
    parameters.oscillator1Feedback = clamp(
        static_cast<float>(algorithm->v[kParamOscillator1Feedback]) / 1000.0f,
        -1.0f, 1.0f);
    parameters.oscillator2Feedback = clamp(
        static_cast<float>(algorithm->v[kParamOscillator2Feedback]) / 1000.0f,
        -1.0f, 1.0f);
    parameters.oscillator1CrossModulation = clamp(
        static_cast<float>(algorithm->v[kParamOscillator1CvAmount]) / 1000.0f,
        -1.0f, 1.0f);
    parameters.oscillator2CrossModulation = clamp(
        static_cast<float>(algorithm->v[kParamOscillator2CvAmount]) / 1000.0f,
        -1.0f, 1.0f);
    parameters.changeCvAmount = clamp(
        static_cast<float>(algorithm->v[kParamChangeCvAmount]) / 1000.0f,
        -1.0f, 1.0f);
    parameters.maximal127Mode = algorithm->v[kParamStepsMode] != 0;
    parameters.doubleEdgeClock = algorithm->v[kParamClockRate] != 0;
    if (algorithm->v[kParamDacTaps] == 0) {
        parameters.dacMsbTap = 7u;
        parameters.dacMiddleTap = 6u;
        parameters.dacLsbTap = 5u;
    } else {
        parameters.dacMsbTap = 6u;
        parameters.dacMiddleTap = 3u;
        parameters.dacLsbTap = 1u;
    }
    parameters.filterFeedback = clamp(
        static_cast<float>(algorithm->v[kParamSteppedToCutoff]) / 1000.0f,
        -1.0f, 1.0f);
    parameters.externalCutoffModulation = clamp(
        static_cast<float>(algorithm->v[kParamFilterCvAmount]) / 1000.0f,
        -1.0f, 1.0f);
    parameters.resonanceCvAmount = clamp(
        static_cast<float>(algorithm->v[kParamResonanceCvAmount]) / 1000.0f,
        -1.0f, 1.0f);
    parameters.mixCvAmount = clamp(
        static_cast<float>(algorithm->v[kParamMixCvAmount]) / 1000.0f,
        -1.0f, 1.0f);
    parameters.inputDrive = clamp(
        static_cast<float>(algorithm->v[kParamInputDrive]) / 100.0f,
        0.25f, 4.0f);
    parameters.safetyLimit = algorithm->v[kParamOutputLimit] != 0;

    const int quality = algorithm->v[kParamQuality];
    parameters.quality = quality <= 0 ? burl::QualityEco
        : (quality >= 2 ? burl::QualityHigh : burl::QualityNormal);
    parameters.auxASource = static_cast<burl::AuxSource>(
        algorithm->v[kParamAuxASource]);
    parameters.auxBSource = static_cast<burl::AuxSource>(
        algorithm->v[kParamAuxBSource]);

    parameters.useExternalOscillator1Cv =
        algorithm->v[kParamOscillator1CvInput] > 0;
    parameters.useExternalOscillator2Cv =
        algorithm->v[kParamOscillator2CvInput] > 0;
    parameters.useExternalClock = algorithm->v[kParamClockInput] > 0;

    algorithm->voice->setParameters(parameters);
}

void parameterChanged(_NT_algorithm* self, int parameter) {
    BurlAlgorithm* algorithm = static_cast<BurlAlgorithm*>(self);
    applyParameters(algorithm);
    if (algorithm->v == nullptr)
        return;

    if (parameter == kParamSeed) {
        algorithm->voice->setSeed(
            static_cast<uint8_t>(algorithm->v[kParamSeed]));
        algorithm->voice->reset();
    } else if (parameter == kParamReseed) {
        const int16_t reseed = algorithm->v[kParamReseed];
        if (reseed != 0 && algorithm->lastReseed == 0) {
            algorithm->voice->setSeed(
                static_cast<uint8_t>(algorithm->v[kParamSeed]));
            algorithm->voice->reset();
        }
        algorithm->lastReseed = reseed;
    }
    algorithm->displayStatus = algorithm->voice->status();
}

const float* inputBus(const BurlAlgorithm* algorithm, float* busFrames,
                      int parameter, int numFrames) {
    const int bus = algorithm->v[parameter];
    if (bus <= 0 || bus > kNT_lastBus)
        return nullptr;
    return busFrames + (bus - 1) * numFrames;
}

void step(_NT_algorithm* self, float* busFrames, int numFramesBy4) {
    BurlAlgorithm* algorithm = static_cast<BurlAlgorithm*>(self);
    const int numFrames = numFramesBy4 * 4;
    if (numFrames <= 0 || algorithm->v == nullptr)
        return;

    const float* inputs[arrayCount(kInputParameters)];
    for (size_t index = 0; index < arrayCount(kInputParameters); ++index) {
        inputs[index] = inputBus(algorithm, busFrames, kInputParameters[index],
                                 numFrames);
    }

    for (int frame = 0; frame < numFrames; ++frame) {
        burl::VoiceInputs voiceInputs;
        voiceInputs.oscillator1Cv = inputs[0] != nullptr ? inputs[0][frame] : 0.0f;
        voiceInputs.oscillator2Cv = inputs[1] != nullptr ? inputs[1][frame] : 0.0f;
        voiceInputs.cutoffCv = inputs[2] != nullptr ? inputs[2][frame] : 0.0f;
        voiceInputs.filterAudio = inputs[3] != nullptr ? inputs[3][frame] : 0.0f;
        voiceInputs.externalClock = inputs[4] != nullptr ? inputs[4][frame] : 0.0f;
        voiceInputs.changeCv = inputs[5] != nullptr ? inputs[5][frame] : 0.0f;
        voiceInputs.resonanceCv = inputs[6] != nullptr ? inputs[6][frame] : 0.0f;
        voiceInputs.mixCv = inputs[7] != nullptr ? inputs[7][frame] : 0.0f;
        voiceInputs.reset = inputs[8] != nullptr ? inputs[8][frame] : 0.0f;

        const burl::VoiceOutputs outputs = algorithm->voice->process(voiceInputs);
        const float values[] = {
            outputs.lowPass, outputs.bandPass, outputs.highPass,
            outputs.steppedCv, outputs.pwm, outputs.registerXor,
            outputs.auxA, outputs.auxB
        };

        for (size_t outputIndex = 0;
             outputIndex < arrayCount(kOutputParameters); ++outputIndex) {
            const int parameter = kOutputParameters[outputIndex];
            const int bus = algorithm->v[parameter];
            if (bus <= 0 || bus > kNT_lastBus)
                continue;
            float* destination = busFrames + (bus - 1) * numFrames;
            const bool replace = algorithm->v[parameter + 1] != 0;
            destination[frame] = replace
                ? values[outputIndex]
                : destination[frame] + values[outputIndex];
        }
    }

    algorithm->displayStatus = algorithm->voice->status();
}

bool draw(_NT_algorithm* self) {
    BurlAlgorithm* algorithm = static_cast<BurlAlgorithm*>(self);
    const burl::VoiceStatus status = algorithm->displayStatus;
    NT_drawText(0, 18, "Burl");

    char bits[9];
    for (unsigned int bit = 0u; bit < 8u; ++bit) {
        bits[bit] = ((status.patternState >> (7u - bit)) & 1u) != 0u
            ? '1' : '0';
    }
    bits[8] = '\0';
    NT_drawText(0, 30, bits);

    char voltage[kNT_parameterStringSize];
    int length = NT_floatToString(voltage, status.steppedCv, 2);
    voltage[length++] = ' ';
    voltage[length++] = 'V';
    voltage[length] = '\0';
    NT_drawText(80, 30, voltage);
    NT_drawText(0, 42, status.externalClock ? "External clock" : "Internal clock");
    NT_drawText(0, 54, status.active ? "Active" : "Ready");
    return false;
}

int appendText(char* buffer, int length, const char* text) {
    while (*text != '\0')
        buffer[length++] = *text++;
    buffer[length] = '\0';
    return length;
}

int parameterString(_NT_algorithm*, int parameter, int value, char* buffer) {
    if (parameter == kParamOscillator1Frequency
        || parameter == kParamOscillator2Frequency
        || parameter == kParamCutoff) {
        const bool cutoff = parameter == kParamCutoff;
        const float hertz = cutoff
            ? frequencyFromControl(value, 4.0f, 16000.0f)
            : frequencyFromControl(value, 0.02f, 8500.0f);
        const int decimals = hertz < 10.0f ? 2 : (hertz < 100.0f ? 1 : 0);
        int length = NT_floatToString(buffer, hertz, decimals);
        return appendText(buffer, length, " Hz");
    }
    if (parameter == kParamInputDrive) {
        int length = NT_floatToString(
            buffer, static_cast<float>(value) / 100.0f, 2);
        return appendText(buffer, length, "x");
    }
    if (parameter == kParamReseed)
        return appendText(buffer, 0, value != 0 ? "Reset" : "Off");
    return 0;
}

// ThBu is the frozen Burl v1 preset identity.
static const _NT_factory kFactory = {
    NT_MULTICHAR('T', 'h', 'B', 'u'),
    "Burl",
    "Chaotic dual-oscillator voice with feedback pattern generator and filter",
    0,
    nullptr,
    nullptr,
    nullptr,
    calculateRequirements,
    construct,
    parameterChanged,
    step,
    draw,
    nullptr,
    nullptr,
    kNT_tagInstrument | kNT_tagFilterEQ,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    nullptr,
    parameterString,
};

} // namespace

extern "C" uintptr_t pluginEntry(_NT_selector selector, uint32_t data) {
    switch (selector) {
    case kNT_selector_version:
        return kNT_apiVersionCurrent;
    case kNT_selector_numFactories:
        return 1;
    case kNT_selector_factoryInfo:
        return data == 0 ? reinterpret_cast<uintptr_t>(&kFactory) : 0;
    }
    return 0;
}
