#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <cmath>
#include <limits>

namespace
{
constexpr auto stateId = "GrannyState";

juce::StringArray directionChoices()
{
    return { "Forward", "Reverse", "Random reverse", "Splice jump" };
}

juce::StringArray pitchChoices()
{
    return { "Original", "Random +12", "Random +/-12" };
}

juce::StringArray stretchEngineChoices()
{
    return { "Hybrid", "Spectral" };
}

juce::StringArray outputMonitorChoices()
{
    return { "Full", "Errors Only" };
}

double sanitiseSampleRate (double requestedRate, double fallbackRate)
{
    constexpr std::array<double, 6> standardRates { 44100.0, 48000.0, 88200.0, 96000.0, 176400.0, 192000.0 };

    if (! std::isfinite (requestedRate) || requestedRate < 8000.0 || requestedRate > 384000.0)
        return fallbackRate > 0.0 ? fallbackRate : 44100.0;

    auto nearestRate = standardRates.front();
    auto nearestDistance = std::abs (requestedRate - nearestRate);

    for (auto rate : standardRates)
    {
        const auto distance = std::abs (requestedRate - rate);
        if (distance < nearestDistance)
        {
            nearestRate = rate;
            nearestDistance = distance;
        }
    }

    const auto deviation = nearestDistance / nearestRate;
    if (deviation <= 0.005)
        return nearestRate;

    return fallbackRate > 0.0 ? fallbackRate : requestedRate;
}

const std::vector<GrannyAudioProcessor::PresetDefinition>& presetDefinitions()
{
    static const std::vector<GrannyAudioProcessor::PresetDefinition> presets
    {
        { "Manual",        8.0f,  false, 0.50f, 180.0f,  8.0f, 1.00f, 1.00f, 0.00f, 0.08f, 0.05f, 0.10f, 0.15f, 0.35f, 0.60f, 0, 0,  0.0f },
        { "Tape Flutter",  1.2f,  false, 0.34f,  34.0f, 20.0f, 0.82f, 1.05f, 0.10f, 0.56f, 0.12f, 0.31f, 0.22f, 0.22f, 0.44f, 0, 0, -2.2f },
        { "Micro Slice",   0.28f, true,  0.14f,  16.0f, 61.0f, 0.93f, 1.70f, 0.24f, 0.28f, 0.08f, 0.34f, 0.34f, 0.17f, 0.72f, 0, 0,  0.0f },
        { "Needle Loop",   0.19f, true,  0.73f,  22.0f, 66.0f, 0.69f, 2.10f, 0.34f, 0.42f, 0.06f, 0.44f, 0.24f, 0.28f, 0.78f, 2, 0, -8.5f },
        { "Pixel Repeater",0.38f, false, 0.47f,  12.0f, 68.0f, 1.36f, 1.28f, 0.92f, 0.24f, 0.12f, 0.84f, 0.24f, 0.12f, 0.61f, 3, 1,  0.0f },
        { "Slip Mosaic",   0.48f, true,  0.28f,  36.0f, 38.0f, 0.86f, 2.10f, 0.72f, 0.46f, 0.18f, 0.90f, 0.54f, 0.43f, 0.76f, 3, 2,  2.5f },
        { "Broken Halo",   5.9f,  false, 0.55f, 180.0f, 11.0f, 0.62f, 2.90f, 0.58f, 0.70f, 0.54f, 0.56f, 0.84f, 0.77f, 0.68f, 2, 2, -5.5f },
        { "Reverse Ash",   2.8f,  false, 0.84f,  88.0f, 19.0f, 0.68f, 1.95f, 0.82f, 0.52f, 0.18f, 0.82f, 0.42f, 0.64f, 0.66f, 3, 0, -9.5f },
        { "Dust Bloom",    8.8f,  false, 0.39f, 280.0f,  5.2f, 0.52f, 3.20f, 0.32f, 0.64f, 0.68f, 0.34f, 0.98f, 0.86f, 0.70f, 0, 2,  4.0f },
        { "Count Fracture",0.58f, true,  0.61f,  28.0f, 44.0f, 1.11f, 1.52f, 0.96f, 0.34f, 0.18f, 0.98f, 0.34f, 0.31f, 0.74f, 3, 1, -3.0f },
        { "Glass Rain",   10.5f,  false, 0.71f, 210.0f, 12.0f, 0.68f, 2.45f, 0.44f, 0.34f, 0.46f, 0.44f, 0.80f, 0.62f, 0.60f, 2, 2,  0.0f },
        { "Frozen Choir", 16.0f,  true,  0.56f, 460.0f,  6.8f, 0.48f, 3.50f, 0.26f, 0.58f, 0.74f, 0.22f, 0.92f, 0.82f, 0.74f, 0, 2,  8.0f },
        { "Reverse Bloom", 8.9f,  false, 0.78f, 260.0f,  6.4f, 0.71f, 2.65f, 0.36f, 0.38f, 0.42f, 0.28f, 0.88f, 0.71f, 0.68f, 1, 0, -1.5f },
        { "Shimmer Arc",   6.2f,  false, 0.46f, 150.0f, 10.5f, 1.04f, 1.82f, 0.28f, 0.24f, 0.24f, 0.34f, 0.62f, 0.58f, 0.62f, 0, 1,  9.0f },
        { "Dust Loop",     2.8f,  true,  0.22f,  72.0f, 26.0f, 0.58f, 2.20f, 0.52f, 0.62f, 0.18f, 0.72f, 0.38f, 0.34f, 0.57f, 2, 0, -6.0f },
        { "Cathedral",    18.5f,  false, 0.61f, 560.0f,  3.1f, 0.39f, 3.70f, 0.18f, 0.78f, 0.84f, 0.18f, 1.00f, 0.90f, 0.76f, 0, 2, -3.0f },
        { "Strobe Mist",   0.72f, false, 0.51f,  18.0f, 48.0f, 1.42f, 0.78f, 0.76f, 0.24f, 0.10f, 0.86f, 0.16f, 0.16f, 0.50f, 2, 1,  0.0f },
        { "Octave Drift",  4.8f,  false, 0.66f, 120.0f, 15.0f, 1.24f, 2.05f, 0.42f, 0.40f, 0.24f, 0.40f, 0.66f, 0.47f, 0.59f, 0, 2, 12.0f }
    };

    return presets;
}

juce::StringArray presetNames()
{
    juce::StringArray names;

    for (const auto& preset : presetDefinitions())
        names.add (preset.name);

    return names;
}
}

GrannyAudioProcessor::GrannyAudioProcessor()
    : AudioProcessor (BusesProperties()
        .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
        .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      parameters (*this, nullptr, stateId, createParameterLayout())
{
    auto connect = [this] (const juce::String& id, std::atomic<float>*& target)
    {
        target = parameters.getRawParameterValue (id);
        parameters.addParameterListener (id, this);
    };

    connect ("bufferSeconds", bufferSecondsParam);
    connect ("freeze", freezeParam);
    connect ("scrub", scrubParam);
    connect ("grainSize", grainSizeParam);
    connect ("density", densityParam);
    connect ("speed", speedParam);
    connect ("stretch", stretchParam);
    connect ("splice", spliceParam);
    connect ("age", ageParam);
    connect ("blur", blurParam);
    connect ("slip", slipParam);
    connect ("bloom", bloomParam);
    connect ("regen", regenParam);
    connect ("mix", mixParam);
    connect ("stretchEngine", stretchEngineParam);
    connect ("monitor", outputMonitorParam);
    connect ("direction", directionParam);
    connect ("pitchMode", pitchModeParam);
    connect ("clock", clockParam);
}

GrannyAudioProcessor::~GrannyAudioProcessor()
{
    for (const auto& id : { "bufferSeconds", "freeze", "scrub", "grainSize", "density", "speed", "stretch", "splice",
                            "age", "blur", "slip", "bloom",
                            "regen", "mix", "stretchEngine", "monitor", "direction", "pitchMode", "clock" })
        parameters.removeParameterListener (id, this);
}

const juce::String GrannyAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool GrannyAudioProcessor::acceptsMidi() const
{
    return false;
}

bool GrannyAudioProcessor::producesMidi() const
{
    return false;
}

bool GrannyAudioProcessor::isMidiEffect() const
{
    return false;
}

double GrannyAudioProcessor::getTailLengthSeconds() const
{
    return 20.0;
}

int GrannyAudioProcessor::getNumPrograms()
{
    return static_cast<int> (getPresetDefinitions().size());
}

int GrannyAudioProcessor::getCurrentProgram()
{
    return currentPresetIndex;
}

void GrannyAudioProcessor::setCurrentProgram (int index)
{
    applyPreset (index);
}

const juce::String GrannyAudioProcessor::getProgramName (int index)
{
    return getPresetNames()[index];
}

void GrannyAudioProcessor::changeProgramName (int, const juce::String&)
{
}

void GrannyAudioProcessor::prepareToPlay (double sampleRate, int)
{
    resetEngineState (sanitiseSampleRate (sampleRate, currentSampleRate));
    refreshCachedParameters();
}

void GrannyAudioProcessor::releaseResources()
{
}

bool GrannyAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    const auto input = layouts.getMainInputChannelSet();
    const auto output = layouts.getMainOutputChannelSet();

    if (output != juce::AudioChannelSet::stereo())
        return false;

    return input == juce::AudioChannelSet::mono() || input == juce::AudioChannelSet::stereo();
}

void GrannyAudioProcessor::parameterChanged (const juce::String& parameterID, float)
{
    if (parameterID == "stretchEngine")
        spectralResetPending.store (true);

    refreshCachedParameters();
}

const std::vector<GrannyAudioProcessor::PresetDefinition>& GrannyAudioProcessor::getPresetDefinitions()
{
    return presetDefinitions();
}

const juce::StringArray& GrannyAudioProcessor::getPresetNames()
{
    static const auto names = presetNames();
    return names;
}

void GrannyAudioProcessor::setParameterValue (const juce::String& parameterID, float plainValue)
{
    if (auto* parameter = parameters.getParameter (parameterID))
        parameter->setValueNotifyingHost (parameter->convertTo0to1 (plainValue));
}

void GrannyAudioProcessor::applyPreset (int presetIndex)
{
    const auto& presets = getPresetDefinitions();
    const auto safeIndex = juce::jlimit (0, static_cast<int> (presets.size()) - 1, presetIndex);
    const auto& preset = presets[static_cast<size_t> (safeIndex)];

    setParameterValue ("bufferSeconds", preset.bufferSeconds);
    setParameterValue ("freeze", preset.freeze ? 1.0f : 0.0f);
    setParameterValue ("scrub", preset.scrub);
    setParameterValue ("grainSize", preset.grainSizeMs);
    setParameterValue ("density", preset.densityHz);
    setParameterValue ("speed", preset.speed);
    setParameterValue ("stretch", preset.stretch);
    setParameterValue ("splice", preset.spliceAmount);
    setParameterValue ("age", preset.age);
    setParameterValue ("blur", preset.blur);
    setParameterValue ("slip", preset.slip);
    setParameterValue ("bloom", preset.bloom);
    setParameterValue ("regen", preset.regen);
    setParameterValue ("mix", preset.mix);
    setParameterValue ("stretchEngine", static_cast<float> (preset.stretchEngine));
    setParameterValue ("direction", static_cast<float> (preset.direction));
    setParameterValue ("pitchMode", static_cast<float> (preset.pitchMode));
    setParameterValue ("clock", preset.clockSemitones);

    currentPresetIndex = safeIndex;
    updateHostDisplay();
}

void GrannyAudioProcessor::getWaveformSnapshot (std::vector<float>& destination, int numPoints) const
{
    destination.clear();

    if (numPoints <= 0 || delayBufferSize <= 0 || currentSampleRate <= 0.0)
        return;

    const auto bufferSeconds = juce::jlimit (0.1f, 20.0f, bufferSecondsParam != nullptr ? bufferSecondsParam->load() : 8.0f);
    const auto activeSamples = juce::jlimit (1, delayBufferSize, static_cast<int> (bufferSeconds * currentSampleRate));
    const auto step = juce::jmax (1, activeSamples / numPoints);

    destination.resize (static_cast<size_t> (numPoints), 0.0f);

    if (waveformDisplayBuffer == nullptr)
        return;

    const auto anchor = lastFreezeState ? visualFreezeWritePosition.load (std::memory_order_relaxed)
                                        : visualWritePosition.load (std::memory_order_relaxed);
    auto readStart = anchor - activeSamples;

    for (int point = 0; point < numPoints; ++point)
    {
        float peak = 0.0f;
        const auto begin = readStart + (point * step);
        const auto end = juce::jmin (begin + step, readStart + activeSamples);

        for (int sample = begin; sample < end; ++sample)
        {
            const auto index = wrapSampleIndex (sample);
            const auto mono = waveformDisplayBuffer[static_cast<size_t> (index)].load (std::memory_order_relaxed);
            peak = juce::jmax (peak, mono);
        }

        destination[static_cast<size_t> (point)] = peak;
    }
}

float GrannyAudioProcessor::getScrubPosition() const
{
    return scrubParam != nullptr ? scrubParam->load() : 0.5f;
}

bool GrannyAudioProcessor::isFrozen() const
{
    return freezeParam != nullptr && freezeParam->load() >= 0.5f;
}

void GrannyAudioProcessor::resetEngineState (double newSampleRate)
{
    currentSampleRate = sanitiseSampleRate (newSampleRate, currentSampleRate);
    delayBufferSize = juce::jmax (1, static_cast<int> (std::ceil (20.0 * currentSampleRate)));
    spectralFftSize = 1 << spectralFftOrder;
    spectralHopSamples = spectralFftSize / 8;
    spectralFft = std::make_unique<juce::dsp::FFT> (spectralFftOrder);
    spectralWindow.resize (static_cast<size_t> (spectralFftSize));
    spectralAnalysisMagnitudes.resize (static_cast<size_t> (spectralFftSize / 2 + 1), 0.0f);
    waveformDisplayBuffer = std::make_unique<std::atomic<float>[]> (static_cast<size_t> (delayBufferSize));

    for (int i = 0; i < spectralFftSize; ++i)
        spectralWindow[static_cast<size_t> (i)] = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * (float) i / (float) (spectralFftSize - 1));

    delayBuffer.setSize (2, delayBufferSize);
    delayBuffer.clear();

    for (int i = 0; i < delayBufferSize; ++i)
        waveformDisplayBuffer[static_cast<size_t> (i)].store (0.0f, std::memory_order_relaxed);

    prepareSpectralBuffers();

    for (auto& grain : grains)
        grain = {};

    for (auto& segment : stretchSegments)
        segment = {};

    writePosition = 0;
    freezeWritePosition = 0;
    feedbackSampleLeft = 0.0f;
    feedbackSampleRight = 0.0f;
    feedbackFilterLeft = 0.0f;
    feedbackFilterRight = 0.0f;
    blurDelayA = {};
    blurDelayB = {};
    tonalLowState = {};
    tonalMidLowState = {};
    transientLowState = {};
    artefactLowState = {};
    clearSpectralState (true);
    launchPhase = 0.0f;
    stretchLaunchPhase = 0.0f;
    transientLaunchPhase = 0.0f;
    scanPosition = 0.0;
    stretchAnalysisPosition = 0.0;
    stretchReferencePosition = 0.0;
    transientAnalysisPosition = 0.0;
    transientReferencePosition = 0.0;
    wowPhase = 0.0f;
    flutterPhase = 0.0f;
    stretchTransientEnvelope = 0.0f;
    lastScrubValue = 0.5f;
    scanPositionValid = false;
    stretchAnalysisValid = false;
    stretchReferenceValid = false;
    transientAnalysisValid = false;
    transientReferenceValid = false;
    lastFreezeState = false;
    spectralResetPending.store (false);
    visualWritePosition.store (0, std::memory_order_relaxed);
    visualFreezeWritePosition.store (0, std::memory_order_relaxed);
}

void GrannyAudioProcessor::refreshCachedParameters()
{
    updateFreezeState();
}

void GrannyAudioProcessor::prepareSpectralBuffers()
{
    for (auto& state : spectralStates)
    {
        state = {};
        state.fftBuffer.assign (static_cast<size_t> (juce::jmax (1, spectralFftSize)), {});
        state.ifftBuffer.assign (static_cast<size_t> (juce::jmax (1, spectralFftSize)), {});
        state.outputAccum.assign (static_cast<size_t> (juce::jmax (2, spectralFftSize * 4)), 0.0f);
        state.lastPhase.assign (static_cast<size_t> (juce::jmax (1, spectralFftSize / 2 + 1)), 0.0f);
        state.priorPhase.assign (static_cast<size_t> (juce::jmax (1, spectralFftSize / 2 + 1)), 0.0f);
        state.phaseAccumulator.assign (static_cast<size_t> (juce::jmax (1, spectralFftSize / 2 + 1)), 0.0f);
        state.previousMagnitudes.assign (static_cast<size_t> (juce::jmax (1, spectralFftSize / 2 + 1)), 0.0f);
        state.magnitudes.assign (static_cast<size_t> (juce::jmax (1, spectralFftSize / 2 + 1)), 0.0f);
        state.analysisPhases.assign (static_cast<size_t> (juce::jmax (1, spectralFftSize / 2 + 1)), 0.0f);
        state.lockedPeakPhases.assign (static_cast<size_t> (juce::jmax (1, spectralFftSize / 2 + 1)), 0.0f);
        state.peakPhaseReady.assign (static_cast<size_t> (juce::jmax (1, spectralFftSize / 2 + 1)), 0);
        state.peakOwners.assign (static_cast<size_t> (juce::jmax (1, spectralFftSize / 2 + 1)), 0);
    }
}

void GrannyAudioProcessor::clearSpectralState (bool clearOutput)
{
    spectralAnalysisPosition = 0.0;
    spectralStateValid = false;

    for (auto& state : spectralStates)
    {
        std::fill (state.fftBuffer.begin(), state.fftBuffer.end(), juce::dsp::Complex<float> {});
        std::fill (state.ifftBuffer.begin(), state.ifftBuffer.end(), juce::dsp::Complex<float> {});
        std::fill (state.lastPhase.begin(), state.lastPhase.end(), 0.0f);
        std::fill (state.priorPhase.begin(), state.priorPhase.end(), 0.0f);
        std::fill (state.phaseAccumulator.begin(), state.phaseAccumulator.end(), 0.0f);
        std::fill (state.previousMagnitudes.begin(), state.previousMagnitudes.end(), 0.0f);
        std::fill (state.magnitudes.begin(), state.magnitudes.end(), 0.0f);
        std::fill (state.analysisPhases.begin(), state.analysisPhases.end(), 0.0f);
        std::fill (state.lockedPeakPhases.begin(), state.lockedPeakPhases.end(), 0.0f);
        std::fill (state.peakPhaseReady.begin(), state.peakPhaseReady.end(), static_cast<uint8_t> (0));
        std::fill (state.peakOwners.begin(), state.peakOwners.end(), 0);

        if (clearOutput)
        {
            std::fill (state.outputAccum.begin(), state.outputAccum.end(), 0.0f);
            state.outputIndex = 0;
        }
    }
}

void GrannyAudioProcessor::updateFreezeState()
{
    const bool freezeEnabled = freezeParam != nullptr && freezeParam->load() >= 0.5f;

    if (freezeEnabled && ! lastFreezeState)
    {
        freezeWritePosition = writePosition;
        visualFreezeWritePosition.store (freezeWritePosition, std::memory_order_relaxed);
    }

    lastFreezeState = freezeEnabled;
}

int GrannyAudioProcessor::wrapSampleIndex (int index) const
{
    jassert (delayBufferSize > 0);

    while (index < 0)
        index += delayBufferSize;

    while (index >= delayBufferSize)
        index -= delayBufferSize;

    return index;
}

double GrannyAudioProcessor::wrapPosition (double position) const
{
    if (delayBufferSize <= 0)
        return 0.0;

    while (position < 0.0)
        position += static_cast<double> (delayBufferSize);

    while (position >= static_cast<double> (delayBufferSize))
        position -= static_cast<double> (delayBufferSize);

    return position;
}

double GrannyAudioProcessor::wrapPositionWithinLoop (double position, double loopStart, double loopEnd) const
{
    const auto loopLength = juce::jmax (1.0, loopEnd - loopStart);

    while (position < loopStart)
        position += loopLength;

    while (position >= loopEnd)
        position -= loopLength;

    return position;
}

float GrannyAudioProcessor::readMonoFromDelayLineCubic (double position) const
{
    return 0.5f * (readFromDelayLineCubic (0, position) + readFromDelayLineCubic (1, position));
}

float GrannyAudioProcessor::measureTransientStrength (double position, double scanWindowStart, double scanWindowEnd) const
{
    constexpr int offsetA = 8;
    constexpr int offsetB = 24;

    const auto p0 = wrapPositionWithinLoop (position, scanWindowStart, scanWindowEnd);
    const auto p1 = wrapPositionWithinLoop (position - offsetA, scanWindowStart, scanWindowEnd);
    const auto p2 = wrapPositionWithinLoop (position - offsetB, scanWindowStart, scanWindowEnd);
    const auto p3 = wrapPositionWithinLoop (position + offsetA, scanWindowStart, scanWindowEnd);

    const auto s0 = readMonoFromDelayLineCubic (p0);
    const auto s1 = readMonoFromDelayLineCubic (p1);
    const auto s2 = readMonoFromDelayLineCubic (p2);
    const auto s3 = readMonoFromDelayLineCubic (p3);

    const auto recentDelta = std::abs (s0 - s1) + 0.6f * std::abs (s1 - s2);
    const auto forwardDelta = std::abs (s3 - s0);
    return recentDelta + forwardDelta * 0.5f;
}

float GrannyAudioProcessor::applyOnePoleLowpass (float input, float cutoffHz, float& state) const
{
    const auto clampedCutoff = juce::jlimit (20.0f, static_cast<float> (currentSampleRate * 0.45), cutoffHz);
    const auto coefficient = 1.0f - std::exp (-juce::MathConstants<float>::twoPi * clampedCutoff
                                              / static_cast<float> (currentSampleRate));
    state += coefficient * (input - state);
    return state;
}

float GrannyAudioProcessor::evaluateStretchAlignment (double candidatePosition, double referencePosition,
                                                      int correlationSamples, double increment,
                                                      double scanWindowStart, double scanWindowEnd) const
{
    const auto sampleCount = juce::jmax (8, correlationSamples);
    auto cross = 0.0f;
    auto candidateEnergy = 0.0f;
    auto referenceEnergy = 0.0f;
    auto derivativeMismatch = 0.0f;
    auto previousCandidate = 0.0f;
    auto previousReference = 0.0f;

    for (int sample = 0; sample < sampleCount; ++sample)
    {
        const auto offset = increment * static_cast<double> (sample);
        const auto candidate = wrapPositionWithinLoop (candidatePosition + offset, scanWindowStart, scanWindowEnd);
        const auto reference = wrapPositionWithinLoop (referencePosition + offset, scanWindowStart, scanWindowEnd);
        const auto candidateMono = readMonoFromDelayLineCubic (candidate);
        const auto referenceMono = readMonoFromDelayLineCubic (reference);
        const auto weight = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi
                                                    * (static_cast<float> (sample) / static_cast<float> (sampleCount)));

        cross += candidateMono * referenceMono * weight;
        candidateEnergy += candidateMono * candidateMono * weight;
        referenceEnergy += referenceMono * referenceMono * weight;

        if (sample > 0)
            derivativeMismatch += std::abs ((candidateMono - previousCandidate) - (referenceMono - previousReference)) * weight;

        previousCandidate = candidateMono;
        previousReference = referenceMono;
    }

    const auto transientCandidate = measureTransientStrength (candidatePosition, scanWindowStart, scanWindowEnd);
    const auto transientReference = measureTransientStrength (referencePosition, scanWindowStart, scanWindowEnd);
    const auto transientMismatch = std::abs (transientCandidate - transientReference);
    const auto normalisedCorrelation = cross / std::sqrt (candidateEnergy * referenceEnergy + 1.0e-9f);

    return normalisedCorrelation - derivativeMismatch * 0.055f - transientMismatch * 0.12f;
}

float GrannyAudioProcessor::readFromDelayLine (int channel, double position) const
{
    const auto wrapped = wrapPosition (position);
    const auto indexA = static_cast<int> (wrapped);
    const auto indexB = wrapSampleIndex (indexA + 1);
    const auto frac = static_cast<float> (wrapped - static_cast<double> (indexA));
    const auto* data = delayBuffer.getReadPointer (channel);

    return juce::jmap (frac, data[indexA], data[indexB]);
}

float GrannyAudioProcessor::readFromDelayLineCubic (int channel, double position) const
{
    const auto wrapped = wrapPosition (position);
    const auto index1 = static_cast<int> (std::floor (wrapped));
    const auto index0 = wrapSampleIndex (index1 - 1);
    const auto index2 = wrapSampleIndex (index1 + 1);
    const auto index3 = wrapSampleIndex (index1 + 2);
    const auto frac = wrapped - static_cast<double> (index1);
    const auto* data = delayBuffer.getReadPointer (channel);

    const auto y0 = static_cast<double> (data[index0]);
    const auto y1 = static_cast<double> (data[wrapSampleIndex (index1)]);
    const auto y2 = static_cast<double> (data[index2]);
    const auto y3 = static_cast<double> (data[index3]);

    const auto a = (-0.5 * y0) + (1.5 * y1) - (1.5 * y2) + (0.5 * y3);
    const auto b = y0 - (2.5 * y1) + (2.0 * y2) - (0.5 * y3);
    const auto c = (-0.5 * y0) + (0.5 * y2);
    const auto d = y1;

    return static_cast<float> (((a * frac + b) * frac + c) * frac + d);
}

float GrannyAudioProcessor::choosePitchRatio()
{
    const auto mode = static_cast<PitchMode> (static_cast<int> (pitchModeParam->load()));

    switch (mode)
    {
        case PitchMode::original:
            return 1.0f;
        case PitchMode::octaveUp:
            return random.nextBool() ? 2.0f : 1.0f;
        case PitchMode::octaveUpDown:
            switch (random.nextInt (3))
            {
                case 0:  return 0.5f;
                case 1:  return 1.0f;
                default: return 2.0f;
            }
    }

    return 1.0f;
}

float GrannyAudioProcessor::chooseDirection()
{
    const auto mode = static_cast<DirectionMode> (static_cast<int> (directionParam->load()));

    switch (mode)
    {
        case DirectionMode::forward:       return 1.0f;
        case DirectionMode::reverse:       return -1.0f;
        case DirectionMode::randomReverse: return random.nextBool() ? -1.0f : 1.0f;
        case DirectionMode::spliceJump:    return random.nextBool() ? -1.0f : 1.0f;
    }

    return 1.0f;
}

void GrannyAudioProcessor::spawnGrain()
{
    auto* freeGrain = std::find_if (grains.begin(), grains.end(), [] (const Grain& grain) { return ! grain.active; });

    if (freeGrain == grains.end() || currentSampleRate <= 0.0 || delayBufferSize <= 0)
        return;

    const auto bufferSeconds = juce::jlimit (0.1f, 20.0f, bufferSecondsParam->load());
    const auto activeBufferSamples = juce::jlimit (1, delayBufferSize, static_cast<int> (bufferSeconds * currentSampleRate));
    const auto scrub = juce::jlimit (0.0f, 1.0f, scrubParam->load());
    const auto grainSamples = juce::jlimit (16, static_cast<int> (currentSampleRate * 2.0),
                                            static_cast<int> ((grainSizeParam->load() * 0.001f) * currentSampleRate));
    const auto stretch = juce::jlimit (0.25f, 4.0f, stretchParam->load());
    const auto clockRatio = std::pow (2.0f, juce::jlimit (-12.0f, 12.0f, clockParam->load()) / 12.0f);
    const auto direction = chooseDirection();
    const auto pitchRatio = choosePitchRatio();
    const auto density = juce::jlimit (0.25f, 64.0f, densityParam->load());
    const auto spliceAmount = juce::jlimit (0.0f, 1.0f, spliceParam->load());
    const auto slip = juce::jlimit (0.0f, 1.0f, slipParam->load());
    const auto bloom = juce::jlimit (0.0f, 1.0f, bloomParam->load());
    const auto microloopBias = juce::jlimit (0.0f, 1.0f, juce::jmap (density, 4.0f, 48.0f, 0.0f, 1.0f) + slip * 0.45f);
    const auto directionMode = static_cast<DirectionMode> (static_cast<int> (directionParam->load()));
    const auto driftSamples = juce::jmax (4, static_cast<int> (static_cast<float> (activeBufferSamples) * (0.01f + 0.08f * microloopBias)));
    const auto jitterSamples = juce::jmax (2, static_cast<int> (static_cast<float> (grainSamples) * (0.1f + 0.65f * microloopBias)));

    const auto relativeOffset = static_cast<int> (scrub * static_cast<float> (activeBufferSamples - 1));
    const auto anchor = lastFreezeState ? freezeWritePosition : writePosition;
    auto basePosition = scanPositionValid
        ? static_cast<int> (scanPosition)
        : anchor - activeBufferSamples + relativeOffset;
    basePosition += random.nextInt ({ -driftSamples, driftSamples + 1 });
    basePosition += random.nextInt ({ -jitterSamples, jitterSamples + 1 });

    const auto loopLengthSamples = juce::jlimit (12,
                                                 juce::jmax (24, grainSamples * 2),
                                                 static_cast<int> (static_cast<float> (grainSamples) * juce::jmap (random.nextFloat(),
                                                                                                                     0.35f,
                                                                                                                     (1.45f + bloom * 1.2f + (stretch - 1.0f) * 0.45f) - 0.65f * microloopBias)));
    auto loopStart = basePosition - loopLengthSamples / 2;
    auto loopEnd = loopStart + loopLengthSamples;

    if (direction < 0.0f)
        basePosition += grainSamples;

    freeGrain->active = true;
    freeGrain->loopStart = static_cast<double> (loopStart);
    freeGrain->loopEnd = static_cast<double> (loopEnd);
    freeGrain->loopLength = static_cast<double> (loopLengthSamples);
    freeGrain->position = wrapPositionWithinLoop (static_cast<double> (basePosition), freeGrain->loopStart, freeGrain->loopEnd);
    freeGrain->increment = static_cast<double> (direction * clockRatio * pitchRatio);
    freeGrain->totalSamples = grainSamples;
    freeGrain->remainingSamples = grainSamples;
    freeGrain->gain = juce::jmap (random.nextFloat(), 0.11f, 0.32f - bloom * 0.06f);
    freeGrain->pan = juce::jmap (random.nextFloat(), -1.0f, 1.0f);
    freeGrain->rightChannelOffset = juce::jmap (random.nextFloat(), -18.0f, 18.0f) * microloopBias;
    freeGrain->previousPosition = freeGrain->position;
    freeGrain->jumpSpan = juce::jmax (8.0, static_cast<double> (loopLengthSamples) * (0.08 + 1.1 * spliceAmount));
    freeGrain->jumpIntervalSamples = directionMode == DirectionMode::spliceJump
        ? juce::jmax (4, static_cast<int> (static_cast<float> (grainSamples) * juce::jmap (spliceAmount, 0.0f, 1.0f, 0.45f, 0.06f)))
        : 0;
    freeGrain->jumpCountdown = freeGrain->jumpIntervalSamples;
    freeGrain->spliceFadeSamples = 0;
    freeGrain->spliceFadeRemaining = 0;
}

void GrannyAudioProcessor::spawnStretchSegment (bool transientMode, int windowSamples, int synthesisHopSamples,
                                                double& analysisPosition, double& referencePosition,
                                                bool& analysisValid, bool& referenceValid,
                                                double scanWindowStart, double scanWindowEnd,
                                                double sourceAdvance, float age, float slip, float bloom)
{
    auto* freeSegment = std::find_if (stretchSegments.begin(), stretchSegments.end(),
                                      [] (const StretchSegment& segment) { return ! segment.active; });

    if (freeSegment == stretchSegments.end() || currentSampleRate <= 0.0 || delayBufferSize <= 0)
        return;

    const auto clockRatio = std::pow (2.0f, juce::jlimit (-12.0f, 12.0f, clockParam->load()) / 12.0f);
    const auto direction = chooseDirection();
    const auto pitchRatio = choosePitchRatio();
    const auto playbackIncrement = static_cast<double> (direction * clockRatio * pitchRatio);
    const auto transientStrength = measureTransientStrength (analysisPosition, scanWindowStart, scanWindowEnd);
    const auto transientAmount = juce::jlimit (0.0f, 1.0f, juce::jmap (transientStrength, 0.015f, 0.22f, 0.0f, 1.0f));
    const auto searchRadius = juce::jmax (transientMode ? 4 : 6, static_cast<int> (static_cast<float> (windowSamples)
                                                               * (transientMode ? 0.022f : 0.04f)
                                                               * (1.0f + slip * (transientMode ? 0.1f : 0.22f) + age * 0.12f)
                                                               * juce::jmap (transientAmount, transientMode ? 0.32f : 0.45f, 1.0f)));
    const auto correlationSamples = juce::jlimit (transientMode ? 12 : 24, transientMode ? 96 : 192,
                                                  static_cast<int> (static_cast<float> (synthesisHopSamples)
                                                                    * juce::jmap (transientAmount, transientMode ? 0.8f : 1.2f,
                                                                                                   transientMode ? 0.5f : 0.7f)));
    const auto searchStep = juce::jmax (1, synthesisHopSamples / (transientMode ? 12 : 8));

    if (! analysisValid)
    {
        analysisPosition = scanPositionValid ? scanPosition : scanWindowStart;
        analysisValid = true;
    }

    auto bestPosition = wrapPositionWithinLoop (analysisPosition, scanWindowStart, scanWindowEnd);

    if (referenceValid)
    {
        auto bestScore = -std::numeric_limits<float>::max();

        for (int offset = -searchRadius; offset <= searchRadius; offset += searchStep)
        {
            const auto candidate = wrapPositionWithinLoop (analysisPosition + static_cast<double> (offset),
                                                           scanWindowStart, scanWindowEnd);
            const auto score = evaluateStretchAlignment (candidate, referencePosition, correlationSamples,
                                                         playbackIncrement, scanWindowStart, scanWindowEnd)
                             - static_cast<float> (std::abs (offset)) * (transientMode ? 0.0016f : 0.0008f);

            if (score > bestScore)
            {
                bestScore = score;
                bestPosition = candidate;
            }
        }
    }

    freeSegment->active = true;
    freeSegment->transient = transientMode;
    freeSegment->position = bestPosition;
    freeSegment->increment = playbackIncrement;
    freeSegment->windowStart = scanWindowStart;
    freeSegment->windowEnd = scanWindowEnd;
    freeSegment->totalSamples = juce::jmax (transientMode ? 24 : 48, static_cast<int> (static_cast<float> (windowSamples)
                                                                  * juce::jmap (transientAmount, transientMode ? 0.32f : 0.42f, 1.0f)));
    freeSegment->remainingSamples = freeSegment->totalSamples;
    freeSegment->gain = juce::jmap (bloom, transientMode ? 0.16f : 0.20f, transientMode ? 0.28f : 0.31f)
                      * juce::jmap (transientAmount, transientMode ? 1.28f : 1.18f, transientMode ? 0.86f : 0.94f);
    freeSegment->stereoOffset = transientMode ? 0.0f
                                              : juce::jmap (random.nextFloat(), -4.0f, 4.0f) * bloom * (1.0f - transientAmount * 0.75f);

    referencePosition = wrapPositionWithinLoop (bestPosition + playbackIncrement * static_cast<double> (synthesisHopSamples),
                                                scanWindowStart, scanWindowEnd);
    referenceValid = true;
    analysisPosition = wrapPositionWithinLoop (bestPosition + sourceAdvance, scanWindowStart, scanWindowEnd);
}

float GrannyAudioProcessor::popSpectralSample (int channel)
{
    auto& state = spectralStates[static_cast<size_t> (juce::jlimit (0, 1, channel))];

    if (state.outputAccum.empty())
        return 0.0f;

    const auto value = state.outputAccum[static_cast<size_t> (state.outputIndex)];
    state.outputAccum[static_cast<size_t> (state.outputIndex)] = 0.0f;
    state.outputIndex = (state.outputIndex + 1) % static_cast<int> (state.outputAccum.size());
    return value;
}

void GrannyAudioProcessor::processSpectralFrame (double analysisPosition, double analysisAdvance,
                                                 double scanWindowStart, double scanWindowEnd,
                                                 int synthesisHopSamples, float pristineBias, float age)
{
    if (spectralFft == nullptr || spectralFftSize <= 0)
        return;

    const auto analysisHop = juce::jlimit (1.0, static_cast<double> (spectralFftSize / 2), std::abs (analysisAdvance));
    const auto synthesisHop = juce::jmax (1, synthesisHopSamples);
    const auto expectedPhaseAdvance = juce::MathConstants<float>::twoPi * static_cast<float> (analysisHop)
                                    / static_cast<float> (spectralFftSize);
    const auto transientTightness = juce::jlimit (0.0f, 1.0f, juce::jmap (stretchTransientEnvelope, 0.01f, 0.16f, 0.0f, 1.0f));

    for (int channel = 0; channel < 2; ++channel)
    {
        auto& state = spectralStates[static_cast<size_t> (channel)];

        if (state.fftBuffer.size() != static_cast<size_t> (spectralFftSize))
            continue;

        state.priorPhase = state.lastPhase;

        for (int i = 0; i < spectralFftSize; ++i)
        {
            const auto readPos = wrapPositionWithinLoop (analysisPosition + static_cast<double> (i - spectralFftSize / 2),
                                                         scanWindowStart, scanWindowEnd);
            const auto sample = readFromDelayLineCubic (channel, readPos) * spectralWindow[static_cast<size_t> (i)];
            state.fftBuffer[static_cast<size_t> (i)] = { sample, 0.0f };
            state.ifftBuffer[static_cast<size_t> (i)] = {};
        }

        spectralFft->perform (state.fftBuffer.data(), state.ifftBuffer.data(), false);

        for (int bin = 0; bin <= spectralFftSize / 2; ++bin)
        {
            const auto value = state.ifftBuffer[static_cast<size_t> (bin)];
            state.magnitudes[static_cast<size_t> (bin)] = std::abs (value);
            state.analysisPhases[static_cast<size_t> (bin)] = std::atan2 (value.imag(), value.real());
            state.peakOwners[static_cast<size_t> (bin)] = bin;
        }

        int currentPeak = 0;
        for (int bin = 1; bin < spectralFftSize / 2; ++bin)
        {
            const auto left = state.magnitudes[static_cast<size_t> (bin - 1)];
            const auto center = state.magnitudes[static_cast<size_t> (bin)];
            const auto right = state.magnitudes[static_cast<size_t> (bin + 1)];

            if (center >= left && center >= right)
                currentPeak = bin;

            state.peakOwners[static_cast<size_t> (bin)] = currentPeak;
        }

        std::fill (state.lockedPeakPhases.begin(), state.lockedPeakPhases.end(), 0.0f);
        std::fill (state.peakPhaseReady.begin(), state.peakPhaseReady.end(), static_cast<uint8_t> (0));

        for (int bin = 0; bin <= spectralFftSize / 2; ++bin)
        {
            const auto magnitude = state.magnitudes[static_cast<size_t> (bin)];
            const auto phase = state.analysisPhases[static_cast<size_t> (bin)];
            const auto previousPhase = state.priorPhase[static_cast<size_t> (bin)];
            auto deltaPhase = phase - previousPhase - expectedPhaseAdvance * static_cast<float> (bin);

            while (deltaPhase < -juce::MathConstants<float>::pi)
                deltaPhase += juce::MathConstants<float>::twoPi;
            while (deltaPhase > juce::MathConstants<float>::pi)
                deltaPhase -= juce::MathConstants<float>::twoPi;

            const auto previousMagnitude = state.previousMagnitudes[static_cast<size_t> (bin)];
            state.lastPhase[static_cast<size_t> (bin)] = phase;
            state.previousMagnitudes[static_cast<size_t> (bin)] = magnitude;
            const auto owner = state.peakOwners[static_cast<size_t> (bin)];
            auto lockedPhase = phase;
            const auto flux = juce::jmax (0.0f, magnitude - previousMagnitude);
            const auto normalisedFlux = flux / (magnitude + 1.0e-5f);
            const auto transientFreeze = juce::jlimit (0.0f, 1.0f, transientTightness
                                                                 * juce::jmap (normalisedFlux, 0.08f, 0.75f, 0.0f, 1.0f));

            if (owner != bin)
            {
                const auto peakPhase = state.analysisPhases[static_cast<size_t> (owner)];
                if (state.peakPhaseReady[static_cast<size_t> (owner)] == 0)
                {
                    const auto peakPreviousPhase = state.priorPhase[static_cast<size_t> (owner)];
                    auto peakDeltaPhase = peakPhase - peakPreviousPhase - expectedPhaseAdvance * static_cast<float> (owner);

                    while (peakDeltaPhase < -juce::MathConstants<float>::pi)
                        peakDeltaPhase += juce::MathConstants<float>::twoPi;
                    while (peakDeltaPhase > juce::MathConstants<float>::pi)
                        peakDeltaPhase -= juce::MathConstants<float>::twoPi;

                    const auto lockedFrequency = expectedPhaseAdvance * static_cast<float> (owner)
                                               + peakDeltaPhase * juce::jmap (pristineBias, 0.92f, 1.0f);
                    state.lockedPeakPhases[static_cast<size_t> (owner)] = state.phaseAccumulator[static_cast<size_t> (owner)]
                                                                         + lockedFrequency
                                                                         * (static_cast<float> (synthesisHop) / static_cast<float> (spectralHopSamples))
                                                                         * (1.0f - transientTightness * (0.14f + age * 0.1f));
                    state.peakPhaseReady[static_cast<size_t> (owner)] = 1;
                }

                const auto relativePhase = phase - peakPhase;
                const auto independentPhase = state.phaseAccumulator[static_cast<size_t> (bin)]
                                            + (expectedPhaseAdvance * static_cast<float> (bin)
                                               + deltaPhase * juce::jmap (pristineBias, 0.92f, 1.0f))
                                            * (static_cast<float> (synthesisHop) / static_cast<float> (spectralHopSamples));
                const auto lockedOwnerPhase = state.lockedPeakPhases[static_cast<size_t> (owner)] + relativePhase;
                const auto lockAmount = juce::jlimit (0.0f, 1.0f,
                                                      juce::jmap (magnitude, 0.0008f, 0.06f, 0.25f, 0.92f)
                                                      * (1.0f - transientFreeze * 0.55f));
                lockedPhase = juce::jmap (lockAmount, independentPhase, lockedOwnerPhase);
            }
            else
            {
                const auto trueFrequency = expectedPhaseAdvance * static_cast<float> (bin)
                                         + deltaPhase * juce::jmap (pristineBias, 0.92f, 1.0f);
                state.lockedPeakPhases[static_cast<size_t> (bin)] = state.phaseAccumulator[static_cast<size_t> (bin)]
                                                                   + trueFrequency
                                                                   * (static_cast<float> (synthesisHop) / static_cast<float> (spectralHopSamples))
                                                                   * (1.0f - transientTightness * (0.18f + age * 0.12f));
                state.peakPhaseReady[static_cast<size_t> (bin)] = 1;
                lockedPhase = state.lockedPeakPhases[static_cast<size_t> (bin)];
            }

            if (transientFreeze > 0.001f)
                lockedPhase = juce::jmap (transientFreeze, lockedPhase, phase);

            state.phaseAccumulator[static_cast<size_t> (bin)] = lockedPhase;

            state.ifftBuffer[static_cast<size_t> (bin)] = std::polar (magnitude, lockedPhase);

            if (bin > 0 && bin < spectralFftSize / 2)
                state.ifftBuffer[static_cast<size_t> (spectralFftSize - bin)] = std::conj (state.ifftBuffer[static_cast<size_t> (bin)]);
        }

        spectralFft->perform (state.ifftBuffer.data(), state.fftBuffer.data(), true);

        for (int i = 0; i < spectralFftSize; ++i)
        {
            const auto writeIndex = (state.outputIndex + i) % static_cast<int> (state.outputAccum.size());
            const auto sample = state.fftBuffer[static_cast<size_t> (i)].real()
                              * spectralWindow[static_cast<size_t> (i)]
                              / static_cast<float> (spectralFftSize)
                              * (0.55f + pristineBias * 0.35f);
            state.outputAccum[static_cast<size_t> (writeIndex)] += sample;
        }
    }
}

void GrannyAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const auto totalInputChannels = getTotalNumInputChannels();
    const auto totalOutputChannels = getTotalNumOutputChannels();

    for (auto channel = totalInputChannels; channel < totalOutputChannels; ++channel)
        buffer.clear (channel, 0, buffer.getNumSamples());

    if (delayBufferSize <= 0)
        return;

    updateFreezeState();

    const bool freezeEnabled = freezeParam->load() >= 0.5f;
    const auto regen = juce::jlimit (0.0f, 0.99f, regenParam->load());
    const auto wetMix = juce::jlimit (0.0f, 1.0f, mixParam->load());
    const auto dryMix = 1.0f - wetMix;
    const auto density = juce::jlimit (0.25f, 64.0f, densityParam->load());
    const auto speed = juce::jlimit (0.25f, 4.0f, speedParam->load());
    const auto stretch = juce::jlimit (0.25f, 4.0f, stretchParam->load());
    const auto spliceAmount = juce::jlimit (0.0f, 1.0f, spliceParam->load());
    const auto age = juce::jlimit (0.0f, 1.0f, ageParam->load());
    const auto blur = juce::jlimit (0.0f, 1.0f, blurParam->load());
    const auto slip = juce::jlimit (0.0f, 1.0f, slipParam->load());
    const auto bloom = juce::jlimit (0.0f, 1.0f, bloomParam->load());
    const auto stretchEngine = static_cast<StretchEngineMode> (static_cast<int> (stretchEngineParam->load()));
    const auto outputMonitor = static_cast<OutputMonitorMode> (static_cast<int> (outputMonitorParam->load()));
    const auto useSpectralEngine = stretchEngine == StretchEngineMode::spectral;
    const auto stretchDepth = juce::jlimit (0.0f, 1.0f, std::abs (std::log2 (stretch)) / 1.65f + bloom * 0.18f);
    const auto pristineBias = juce::jlimit (0.0f, 1.0f, stretchDepth * (1.0f - juce::jlimit (0.0f, 1.0f, spliceAmount * 0.7f + slip * 0.8f + bloom * 0.45f + age * 0.35f)));
    const auto effectiveDensity = density * juce::jmap (stretch, 0.9f, 1.9f) * juce::jmap (bloom, 1.0f, 1.8f)
                                * juce::jmap (pristineBias, 1.0f, 0.24f);
    const auto launchIncrement = effectiveDensity / static_cast<float> (currentSampleRate);
    const auto damping = juce::jlimit (0.02f, 0.96f, juce::jmap (age, 0.32f, 0.04f) + juce::jmap (regen, 0.04f, 0.26f));
    const auto activeBufferSamples = juce::jlimit (1, delayBufferSize,
                                                   static_cast<int> (juce::jlimit (0.1f, 20.0f, bufferSecondsParam->load()) * currentSampleRate));
    const auto grainSamples = juce::jlimit (16, static_cast<int> (currentSampleRate * 2.0),
                                            static_cast<int> ((grainSizeParam->load() * 0.001f) * currentSampleRate));
    const auto stretchWindowSamples = juce::jlimit (96, juce::jmax (192, activeBufferSamples / 2),
                                                    static_cast<int> (static_cast<float> (grainSamples)
                                                                      * juce::jmap (stretch, 1.4f, 5.2f)
                                                                      * juce::jmap (bloom, 1.0f, 1.55f)
                                                                      * juce::jmap (pristineBias, 1.1f, 1.55f)));
    const auto overlapFactor = juce::jlimit (4, 20, 4 + static_cast<int> (stretchDepth * 8.0f + pristineBias * 5.0f + blur * 1.0f + bloom * 2.0f));
    const auto synthesisHopSamples = juce::jmax (12, stretchWindowSamples / overlapFactor);
    const auto transientWindowSamples = juce::jlimit (24, juce::jmax (48, grainSamples),
                                                      static_cast<int> (static_cast<float> (grainSamples)
                                                                        * juce::jmap (stretchDepth, 0.55f, 1.4f)
                                                                        * juce::jmap (pristineBias, 0.9f, 0.65f)));
    const auto transientHopSamples = juce::jmax (4, transientWindowSamples / juce::jlimit (3, 8, 3 + static_cast<int> (pristineBias * 4.0f)));

    if (spectralResetPending.exchange (false))
        clearSpectralState (true);

    for (int sample = 0; sample < buffer.getNumSamples(); ++sample)
    {
        const auto inL = buffer.getSample (0, sample);
        const auto inR = totalInputChannels > 1 ? buffer.getSample (1, sample) : inL;
        const auto scrub = juce::jlimit (0.0f, 1.0f, scrubParam->load());
        const auto anchor = freezeEnabled ? freezeWritePosition : writePosition;
        const auto scanWindowStart = static_cast<double> (anchor - activeBufferSamples);
        const auto scanWindowEnd = scanWindowStart + static_cast<double> (activeBufferSamples);
        const auto desiredScanPosition = scanWindowStart + static_cast<double> (scrub * static_cast<float> (activeBufferSamples - 1));

        if (! scanPositionValid || std::abs (scrub - lastScrubValue) > 0.001f)
        {
            scanPosition = desiredScanPosition;
            stretchAnalysisPosition = desiredScanPosition;
            stretchReferencePosition = desiredScanPosition;
            transientAnalysisPosition = desiredScanPosition;
            transientReferencePosition = desiredScanPosition;
            spectralAnalysisPosition = desiredScanPosition;
            stretchTransientEnvelope = 0.0f;
            lastScrubValue = scrub;
            scanPositionValid = true;
            stretchAnalysisValid = true;
            stretchReferenceValid = false;
            transientAnalysisValid = true;
            transientReferenceValid = false;
            clearSpectralState (true);
            spectralStateValid = true;
        }

        wowPhase += 0.00011f + age * 0.00016f;
        flutterPhase += 0.0013f + age * 0.0035f;
        const auto wow = std::sin (wowPhase) * (0.2f + age * 1.6f);
        const auto flutter = std::sin (flutterPhase) * (0.05f + age * 0.35f);
        const auto scanAdvance = static_cast<double> ((speed / stretch) + wow + flutter);
        scanPosition = wrapPositionWithinLoop (scanPosition + scanAdvance, scanWindowStart, scanWindowEnd);
        const auto transientStrength = measureTransientStrength (scanPosition, scanWindowStart, scanWindowEnd);
        stretchTransientEnvelope += 0.08f * (transientStrength - stretchTransientEnvelope);
        const auto transientPresence = juce::jlimit (0.0f, 1.0f, juce::jmap (stretchTransientEnvelope, 0.01f, 0.16f, 0.0f, 1.0f));

        float grainLeft = 0.0f;
        float grainRight = 0.0f;
        float stretchLeft = 0.0f;
        float stretchRight = 0.0f;
        float stretchTransientLeft = 0.0f;
        float stretchTransientRight = 0.0f;

        if (! useSpectralEngine)
        {
            for (auto& grain : grains)
            {
                if (! grain.active)
                    continue;

                const auto envPhase = 1.0f - static_cast<float> (grain.remainingSamples)
                                            / static_cast<float> (juce::jmax (1, grain.totalSamples));
                const auto window = 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * juce::jlimit (0.0f, 1.0f, envPhase));
                const auto driftMod = std::sin ((1.0f - envPhase) * juce::MathConstants<float>::twoPi) * (6.0f + density * 0.3f + slip * 18.0f);
                const auto readPosL = grain.position + driftMod;
                const auto readPosR = grain.position + driftMod + grain.rightChannelOffset;
                auto sourceL = readFromDelayLineCubic (0, readPosL);
                auto sourceR = readFromDelayLineCubic (1, readPosR);

                if (grain.spliceFadeRemaining > 0)
                {
                    const auto fade = 1.0f - static_cast<float> (grain.spliceFadeRemaining)
                                            / static_cast<float> (juce::jmax (1, grain.spliceFadeSamples));
                    const auto oldL = readFromDelayLineCubic (0, grain.previousPosition + driftMod);
                    const auto oldR = readFromDelayLineCubic (1, grain.previousPosition + driftMod + grain.rightChannelOffset);
                    sourceL = juce::jmap (fade, oldL, sourceL);
                    sourceR = juce::jmap (fade, oldR, sourceR);
                    grain.previousPosition = wrapPositionWithinLoop (grain.previousPosition + grain.increment, grain.loopStart, grain.loopEnd);
                    --grain.spliceFadeRemaining;
                }
                const auto mono = 0.5f * (sourceL + sourceR);
                const auto leftPan = std::sqrt (0.5f * (1.0f - grain.pan));
                const auto rightPan = std::sqrt (0.5f * (1.0f + grain.pan));
                const auto sampleValue = mono * grain.gain * window;

                grainLeft += sampleValue * leftPan;
                grainRight += sampleValue * rightPan;

                if (grain.jumpIntervalSamples > 0)
                {
                    --grain.jumpCountdown;

                    if (grain.jumpCountdown <= 0)
                    {
                        const auto jumpOffset = juce::jmap (static_cast<double> (random.nextFloat()), -grain.jumpSpan, grain.jumpSpan);
                        grain.previousPosition = grain.position;
                        grain.position = wrapPositionWithinLoop (grain.position + jumpOffset, grain.loopStart, grain.loopEnd);
                        grain.spliceFadeSamples = juce::jmax (4, static_cast<int> (12.0f + bloom * 36.0f + blur * 22.0f));
                        grain.spliceFadeRemaining = grain.spliceFadeSamples;
                        grain.jumpCountdown = juce::jmax (1, static_cast<int> (static_cast<float> (grain.jumpIntervalSamples)
                                                                               * juce::jmap (random.nextFloat(), 0.7f, 1.3f - 0.4f * spliceAmount)));
                    }
                }

                grain.position = wrapPositionWithinLoop (grain.position + grain.increment, grain.loopStart, grain.loopEnd);
                --grain.remainingSamples;

                if (grain.remainingSamples <= 0)
                    grain = {};
            }
        }

        if (! useSpectralEngine)
        {
            stretchLaunchPhase += 1.0f;
            while (stretchLaunchPhase >= static_cast<float> (synthesisHopSamples))
            {
                const auto sourceAdvance = scanAdvance * static_cast<double> (synthesisHopSamples);
                spawnStretchSegment (false, stretchWindowSamples, synthesisHopSamples,
                                     stretchAnalysisPosition, stretchReferencePosition,
                                     stretchAnalysisValid, stretchReferenceValid,
                                     scanWindowStart, scanWindowEnd,
                                     sourceAdvance, age, slip, bloom);
                stretchLaunchPhase -= static_cast<float> (synthesisHopSamples);
            }

            transientLaunchPhase += 1.0f;
            while (transientLaunchPhase >= static_cast<float> (transientHopSamples))
            {
                if (transientPresence > 0.08f || pristineBias > 0.55f)
                {
                    const auto transientSourceAdvance = scanAdvance * static_cast<double> (transientHopSamples)
                                                      * juce::jmap (transientPresence, 0.35f, 0.9f);
                    spawnStretchSegment (true, transientWindowSamples, transientHopSamples,
                                         transientAnalysisPosition, transientReferencePosition,
                                         transientAnalysisValid, transientReferenceValid,
                                         scanWindowStart, scanWindowEnd,
                                         transientSourceAdvance, age, slip, bloom);
                }

                transientLaunchPhase -= static_cast<float> (transientHopSamples);
            }
        }

        if (! useSpectralEngine)
        {
            for (auto& segment : stretchSegments)
            {
                if (! segment.active)
                    continue;

                const auto envPhase = 1.0f - static_cast<float> (segment.remainingSamples)
                                            / static_cast<float> (juce::jmax (1, segment.totalSamples));
                const auto phase = juce::jlimit (0.0f, 1.0f, envPhase);
                const auto window = std::sqrt (juce::jmax (0.0f, 0.5f - 0.5f * std::cos (juce::MathConstants<float>::twoPi * phase)));
                const auto stereoMod = std::sin (phase * juce::MathConstants<float>::twoPi) * bloom * (0.35f + blur * 1.2f);
                const auto readPosL = wrapPositionWithinLoop (segment.position, segment.windowStart, segment.windowEnd);
                const auto readPosR = wrapPositionWithinLoop (segment.position + segment.stereoOffset + stereoMod,
                                                              segment.windowStart, segment.windowEnd);
                const auto sourceL = readFromDelayLineCubic (0, readPosL);
                const auto sourceR = readFromDelayLineCubic (1, readPosR);
                const auto transientDamping = juce::jlimit (0.82f, 1.0f, 1.0f - stretchTransientEnvelope * 0.35f);
                const auto level = segment.gain * window * transientDamping / std::sqrt (static_cast<float> (overlapFactor));

                if (segment.transient)
                {
                    stretchTransientLeft += sourceL * level;
                    stretchTransientRight += sourceR * level;
                }
                else
                {
                    stretchLeft += sourceL * level;
                    stretchRight += sourceR * level;
                }

                segment.position = wrapPositionWithinLoop (segment.position + segment.increment,
                                                           segment.windowStart, segment.windowEnd);
                --segment.remainingSamples;

                if (segment.remainingSamples <= 0)
                    segment = {};
            }
        }
        else
        {
            if (! spectralStateValid)
            {
                spectralAnalysisPosition = scanPosition;
                spectralStateValid = true;
            }

            stretchLaunchPhase += 1.0f;
            while (stretchLaunchPhase >= static_cast<float> (juce::jmax (1, spectralHopSamples)))
            {
                const auto spectralAdvance = scanAdvance * static_cast<double> (juce::jmax (1, spectralHopSamples));
                processSpectralFrame (spectralAnalysisPosition, spectralAdvance,
                                      scanWindowStart, scanWindowEnd,
                                      juce::jmax (1, spectralHopSamples), pristineBias, age);
                spectralAnalysisPosition = wrapPositionWithinLoop (spectralAnalysisPosition + spectralAdvance,
                                                                   scanWindowStart, scanWindowEnd);
                stretchLaunchPhase -= static_cast<float> (juce::jmax (1, spectralHopSamples));
            }

            stretchLeft = popSpectralSample (0);
            stretchRight = popSpectralSample (1);
        }

        if (! freezeEnabled)
        {
            feedbackFilterLeft += damping * (feedbackSampleLeft - feedbackFilterLeft);
            feedbackFilterRight += damping * (feedbackSampleRight - feedbackFilterRight);
            const auto writeL = inL + feedbackFilterLeft * regen;
            const auto writeR = inR + feedbackFilterRight * regen;

            delayBuffer.setSample (0, writePosition, writeL);
            delayBuffer.setSample (1, writePosition, writeR);
            const auto visualSample = 0.5f * (std::abs (writeL) + std::abs (writeR));
            waveformDisplayBuffer[static_cast<size_t> (writePosition)].store (visualSample, std::memory_order_relaxed);
        }

        launchPhase += launchIncrement;
        while (launchPhase >= 1.0f)
        {
            if (! useSpectralEngine)
                spawnGrain();
            launchPhase -= 1.0f;
        }

        const auto directTransientLeft = readFromDelayLineCubic (0, scanPosition);
        const auto directTransientRight = readFromDelayLineCubic (1, scanPosition);

        const auto tonalLowL = applyOnePoleLowpass (stretchLeft, 420.0f, tonalLowState[0]);
        const auto tonalLowR = applyOnePoleLowpass (stretchRight, 420.0f, tonalLowState[1]);
        const auto tonalMidLowL = applyOnePoleLowpass (stretchLeft, 2600.0f, tonalMidLowState[0]);
        const auto tonalMidLowR = applyOnePoleLowpass (stretchRight, 2600.0f, tonalMidLowState[1]);
        const auto transientLowL = applyOnePoleLowpass (directTransientLeft + stretchTransientLeft, 2400.0f, transientLowState[0]);
        const auto transientLowR = applyOnePoleLowpass (directTransientRight + stretchTransientRight, 2400.0f, transientLowState[1]);

        const auto tonalMidL = tonalMidLowL - tonalLowL;
        const auto tonalMidR = tonalMidLowR - tonalLowR;
        const auto tonalHighL = stretchLeft - tonalMidLowL;
        const auto tonalHighR = stretchRight - tonalMidLowR;
        const auto transientHighL = (directTransientLeft + stretchTransientLeft) - transientLowL;
        const auto transientHighR = (directTransientRight + stretchTransientRight) - transientLowR;
        const auto pristineTonalL = tonalLowL + tonalMidL * 0.96f + tonalHighL * (0.42f + pristineBias * 0.26f);
        const auto pristineTonalR = tonalLowR + tonalMidR * 0.96f + tonalHighR * (0.42f + pristineBias * 0.26f);
        const auto directTransientBlend = juce::jmap (pristineBias, 0.12f, 0.02f);
        const auto preservedTransientL = transientHighL * (0.32f + transientPresence * (0.72f + pristineBias * 0.22f))
                                       + transientLowL * transientPresence * 0.16f
                                       + directTransientLeft * directTransientBlend * transientPresence;
        const auto preservedTransientR = transientHighR * (0.32f + transientPresence * (0.72f + pristineBias * 0.22f))
                                       + transientLowR * transientPresence * 0.16f
                                       + directTransientRight * directTransientBlend * transientPresence;

        const auto stretchBlend = juce::jlimit (0.0f, 1.0f, stretchDepth * (0.6f + pristineBias * 0.5f));
        const auto grainBlend = juce::jmap (pristineBias, 0.58f, 0.24f);
        if (useSpectralEngine)
        {
            grainLeft = pristineTonalL * 1.05f + preservedTransientL;
            grainRight = pristineTonalR * 1.05f + preservedTransientR;
        }
        else
        {
            grainLeft = juce::jmap (stretchBlend, grainLeft, grainLeft * grainBlend + pristineTonalL * 1.02f + preservedTransientL);
            grainRight = juce::jmap (stretchBlend, grainRight, grainRight * grainBlend + pristineTonalR * 1.02f + preservedTransientR);
        }

        const auto cleanReferenceLeft = pristineTonalL + preservedTransientL * (0.8f + pristineBias * 0.25f);
        const auto cleanReferenceRight = pristineTonalR + preservedTransientR * (0.8f + pristineBias * 0.25f);

        feedbackSampleLeft = grainLeft;
        feedbackSampleRight = grainRight;

        if (blur > 0.001f)
        {
            const auto blurCoeffA = 0.28f + blur * 0.46f;
            const auto blurCoeffB = 0.18f + blur * 0.34f;

            const auto allpassAOutL = -blurCoeffA * grainLeft + blurDelayA[0];
            const auto allpassAOutR = -blurCoeffA * grainRight + blurDelayA[1];
            blurDelayA[0] = grainLeft + allpassAOutL * blurCoeffA;
            blurDelayA[1] = grainRight + allpassAOutR * blurCoeffA;

            const auto allpassBOutL = -blurCoeffB * allpassAOutL + blurDelayB[0];
            const auto allpassBOutR = -blurCoeffB * allpassAOutR + blurDelayB[1];
            blurDelayB[0] = allpassAOutL + allpassBOutL * blurCoeffB;
            blurDelayB[1] = allpassAOutR + allpassBOutR * blurCoeffB;

            grainLeft = juce::jmap (blur, grainLeft, allpassBOutL);
            grainRight = juce::jmap (blur, grainRight, allpassBOutR);
        }

        if (outputMonitor == OutputMonitorMode::artefacts)
        {
            const auto residualLeft = grainLeft - cleanReferenceLeft;
            const auto residualRight = grainRight - cleanReferenceRight;
            const auto residualLowLeft = applyOnePoleLowpass (residualLeft, 1200.0f, artefactLowState[0]);
            const auto residualLowRight = applyOnePoleLowpass (residualRight, 1200.0f, artefactLowState[1]);
            const auto artefactOnlyLeft = (residualLeft - residualLowLeft) * 3.5f + residualLeft * 1.2f;
            const auto artefactOnlyRight = (residualRight - residualLowRight) * 3.5f + residualRight * 1.2f;

            grainLeft = std::tanh (artefactOnlyLeft);
            grainRight = std::tanh (artefactOnlyRight);
        }

        buffer.setSample (0, sample, inL * dryMix + grainLeft * wetMix);
        buffer.setSample (1, sample, inR * dryMix + grainRight * wetMix);

        if (! freezeEnabled)
        {
            writePosition = wrapSampleIndex (writePosition + 1);
            visualWritePosition.store (writePosition, std::memory_order_relaxed);
        }
    }
}

bool GrannyAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* GrannyAudioProcessor::createEditor()
{
    return new GrannyAudioProcessorEditor (*this);
}

void GrannyAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    if (auto xml = parameters.copyState().createXml())
        copyXmlToBinary (*xml, destData);
}

void GrannyAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    if (auto xmlState = getXmlFromBinary (data, sizeInBytes))
        if (xmlState->hasTagName (parameters.state.getType()))
            parameters.replaceState (juce::ValueTree::fromXml (*xmlState));
}

GrannyAudioProcessor::APVTS::ParameterLayout GrannyAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "bufferSeconds", "Buffer",
        juce::NormalisableRange<float> (0.1f, 20.0f, 0.01f, 0.35f), 8.0f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction ([] (float value, int)
        {
            return juce::String (value, 2) + " s";
        })));

    params.push_back (std::make_unique<juce::AudioParameterBool> ("freeze", "Freeze", false));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "scrub", "Scrub",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.5f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction ([] (float value, int)
        {
            return juce::String (value * 100.0f, 0) + " %";
        })));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "grainSize", "Grain Size",
        juce::NormalisableRange<float> (15.0f, 1500.0f, 1.0f, 0.45f), 180.0f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction ([] (float value, int)
        {
            return juce::String (value, 0) + " ms";
        })));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "density", "Launch",
        juce::NormalisableRange<float> (0.25f, 64.0f, 0.01f, 0.4f), 8.0f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction ([] (float value, int)
        {
            return juce::String (value, 1) + " Hz";
        })));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "speed", "Speed",
        juce::NormalisableRange<float> (0.25f, 4.0f, 0.001f, 0.5f), 1.0f));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "stretch", "Stretch",
        juce::NormalisableRange<float> (0.25f, 4.0f, 0.001f, 0.5f), 1.0f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction ([] (float value, int)
        {
            return juce::String (value, 2) + "x";
        })));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "splice", "Splice",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.0f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction ([] (float value, int)
        {
            return juce::String (value * 100.0f, 0) + " %";
        })));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "age", "Age",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.08f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction ([] (float value, int)
        {
            return juce::String (value * 100.0f, 0) + " %";
        })));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "blur", "Blur",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.05f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction ([] (float value, int)
        {
            return juce::String (value * 100.0f, 0) + " %";
        })));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "slip", "Slip",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.10f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction ([] (float value, int)
        {
            return juce::String (value * 100.0f, 0) + " %";
        })));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "bloom", "Bloom",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.15f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction ([] (float value, int)
        {
            return juce::String (value * 100.0f, 0) + " %";
        })));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "regen", "Regen",
        juce::NormalisableRange<float> (0.0f, 0.99f, 0.001f), 0.35f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction ([] (float value, int)
        {
            return juce::String (value * 100.0f, 0) + " %";
        })));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "mix", "Mix",
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.001f), 0.6f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction ([] (float value, int)
        {
            return juce::String (value * 100.0f, 0) + " %";
        })));

    params.push_back (std::make_unique<juce::AudioParameterChoice> ("stretchEngine", "Stretch Engine", stretchEngineChoices(), 0));
    params.push_back (std::make_unique<juce::AudioParameterChoice> ("monitor", "Monitor", outputMonitorChoices(), 0));
    params.push_back (std::make_unique<juce::AudioParameterChoice> ("direction", "Direction", directionChoices(), 0));
    params.push_back (std::make_unique<juce::AudioParameterChoice> ("pitchMode", "Pitch Mode", pitchChoices(), 0));

    params.push_back (std::make_unique<juce::AudioParameterFloat> (
        "clock", "Clock",
        juce::NormalisableRange<float> (-12.0f, 12.0f, 0.01f), 0.0f,
        juce::AudioParameterFloatAttributes().withStringFromValueFunction ([] (float value, int)
        {
            return (value >= 0.0f ? "+" : "") + juce::String (value, 1) + " st";
        })));

    return { params.begin(), params.end() };
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new GrannyAudioProcessor();
}
