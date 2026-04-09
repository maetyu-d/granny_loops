#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include <algorithm>
#include <array>
#include <vector>

class GrannyAudioProcessor final : public juce::AudioProcessor,
                                   private juce::AudioProcessorValueTreeState::Listener
{
public:
    struct PresetDefinition
    {
        const char* name = "";
        float bufferSeconds = 8.0f;
        bool freeze = false;
        float scrub = 0.5f;
        float grainSizeMs = 180.0f;
        float densityHz = 8.0f;
        float speed = 1.0f;
        float stretch = 1.0f;
        float spliceAmount = 0.0f;
        float age = 0.0f;
        float blur = 0.0f;
        float slip = 0.0f;
        float bloom = 0.0f;
        float regen = 0.35f;
        float mix = 0.6f;
        int direction = 0;
        int pitchMode = 0;
        float clockSemitones = 0.0f;
        int stretchEngine = 0;
    };

    GrannyAudioProcessor();
    ~GrannyAudioProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;
    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    using APVTS = juce::AudioProcessorValueTreeState;
    APVTS parameters;

    static APVTS::ParameterLayout createParameterLayout();
    static const juce::StringArray& getPresetNames();
    void applyPreset (int presetIndex);
    void getWaveformSnapshot (std::vector<float>& destination, int numPoints) const;
    float getScrubPosition() const;
    bool isFrozen() const;

private:
    struct Grain
    {
        bool active = false;
        double position = 0.0;
        double increment = 1.0;
        double loopStart = 0.0;
        double loopEnd = 0.0;
        double loopLength = 1.0;
        double jumpSpan = 0.0;
        double previousPosition = 0.0;
        int totalSamples = 0;
        int remainingSamples = 0;
        int jumpIntervalSamples = 0;
        int jumpCountdown = 0;
        int spliceFadeSamples = 0;
        int spliceFadeRemaining = 0;
        float gain = 0.0f;
        float pan = 0.0f;
        float rightChannelOffset = 0.0f;
    };

    struct StretchSegment
    {
        bool active = false;
        bool transient = false;
        double position = 0.0;
        double increment = 1.0;
        double windowStart = 0.0;
        double windowEnd = 0.0;
        int totalSamples = 0;
        int remainingSamples = 0;
        float gain = 0.0f;
        float stereoOffset = 0.0f;
    };

    enum class DirectionMode
    {
        forward = 0,
        reverse,
        randomReverse,
        spliceJump
    };

    enum class PitchMode
    {
        original = 0,
        octaveUp,
        octaveUpDown
    };

    enum class StretchEngineMode
    {
        hybrid = 0,
        spectral
    };

    enum class OutputMonitorMode
    {
        full = 0,
        artefacts
    };

    struct SpectralChannelState
    {
        std::vector<juce::dsp::Complex<float>> fftBuffer;
        std::vector<juce::dsp::Complex<float>> ifftBuffer;
        std::vector<float> outputAccum;
        std::vector<float> lastPhase;
        std::vector<float> priorPhase;
        std::vector<float> phaseAccumulator;
        std::vector<float> previousMagnitudes;
        std::vector<float> magnitudes;
        std::vector<float> analysisPhases;
        std::vector<float> lockedPeakPhases;
        std::vector<uint8_t> peakPhaseReady;
        std::vector<int> peakOwners;
        int outputIndex = 0;
    };

    void parameterChanged (const juce::String& parameterID, float newValue) override;
    void refreshCachedParameters();
    void updateFreezeState();
    void prepareSpectralBuffers();
    void clearSpectralState (bool clearOutput);
    void spawnGrain();
    void spawnStretchSegment (bool transientMode, int windowSamples, int synthesisHopSamples,
                              double& analysisPosition, double& referencePosition,
                              bool& analysisValid, bool& referenceValid,
                              double scanWindowStart, double scanWindowEnd,
                              double sourceAdvance, float age, float slip, float bloom);
    void processSpectralFrame (double analysisPosition, double analysisAdvance,
                               double scanWindowStart, double scanWindowEnd,
                               int synthesisHopSamples, float pristineBias, float age);
    void resetEngineState (double newSampleRate);
    float readFromDelayLine (int channel, double position) const;
    float readFromDelayLineCubic (int channel, double position) const;
    int wrapSampleIndex (int index) const;
    double wrapPosition (double position) const;
    float choosePitchRatio();
    float chooseDirection();
    void setParameterValue (const juce::String& parameterID, float plainValue);
    static const std::vector<PresetDefinition>& getPresetDefinitions();
    double wrapPositionWithinLoop (double position, double loopStart, double loopEnd) const;
    float readMonoFromDelayLineCubic (double position) const;
    float measureTransientStrength (double position, double scanWindowStart, double scanWindowEnd) const;
    float evaluateStretchAlignment (double candidatePosition, double referencePosition,
                                    int correlationSamples, double increment,
                                    double scanWindowStart, double scanWindowEnd) const;
    float applyOnePoleLowpass (float input, float cutoffHz, float& state) const;
    float popSpectralSample (int channel);

    juce::AudioBuffer<float> delayBuffer;
    std::array<Grain, 64> grains {};
    std::array<StretchSegment, 24> stretchSegments {};
    juce::Random random;

    double currentSampleRate = 44100.0;
    int delayBufferSize = 0;
    int writePosition = 0;
    int freezeWritePosition = 0;
    float feedbackSampleLeft = 0.0f;
    float feedbackSampleRight = 0.0f;
    float feedbackFilterLeft = 0.0f;
    float feedbackFilterRight = 0.0f;
    std::array<float, 2> blurDelayA {};
    std::array<float, 2> blurDelayB {};
    std::array<float, 2> tonalLowState {};
    std::array<float, 2> tonalMidLowState {};
    std::array<float, 2> transientLowState {};
    std::array<float, 2> artefactLowState {};
    std::array<SpectralChannelState, 2> spectralStates {};
    std::unique_ptr<juce::dsp::FFT> spectralFft;
    std::vector<float> spectralWindow;
    std::vector<float> spectralAnalysisMagnitudes;
    std::unique_ptr<std::atomic<float>[]> waveformDisplayBuffer;
    int spectralFftOrder = 11;
    int spectralFftSize = 0;
    int spectralHopSamples = 0;
    double spectralAnalysisPosition = 0.0;
    bool spectralStateValid = false;
    float launchPhase = 0.0f;
    float stretchLaunchPhase = 0.0f;
    float transientLaunchPhase = 0.0f;
    double scanPosition = 0.0;
    double stretchAnalysisPosition = 0.0;
    double stretchReferencePosition = 0.0;
    double transientAnalysisPosition = 0.0;
    double transientReferencePosition = 0.0;
    float wowPhase = 0.0f;
    float flutterPhase = 0.0f;
    float stretchTransientEnvelope = 0.0f;
    float lastScrubValue = 0.5f;
    bool scanPositionValid = false;
    bool stretchAnalysisValid = false;
    bool stretchReferenceValid = false;
    bool transientAnalysisValid = false;
    bool transientReferenceValid = false;
    bool lastFreezeState = false;
    std::atomic<bool> spectralResetPending { false };
    std::atomic<int> visualWritePosition { 0 };
    std::atomic<int> visualFreezeWritePosition { 0 };
    int currentPresetIndex = 0;

    std::atomic<float>* bufferSecondsParam = nullptr;
    std::atomic<float>* freezeParam = nullptr;
    std::atomic<float>* scrubParam = nullptr;
    std::atomic<float>* grainSizeParam = nullptr;
    std::atomic<float>* densityParam = nullptr;
    std::atomic<float>* speedParam = nullptr;
    std::atomic<float>* stretchParam = nullptr;
    std::atomic<float>* spliceParam = nullptr;
    std::atomic<float>* ageParam = nullptr;
    std::atomic<float>* blurParam = nullptr;
    std::atomic<float>* slipParam = nullptr;
    std::atomic<float>* bloomParam = nullptr;
    std::atomic<float>* regenParam = nullptr;
    std::atomic<float>* mixParam = nullptr;
    std::atomic<float>* stretchEngineParam = nullptr;
    std::atomic<float>* outputMonitorParam = nullptr;
    std::atomic<float>* directionParam = nullptr;
    std::atomic<float>* pitchModeParam = nullptr;
    std::atomic<float>* clockParam = nullptr;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GrannyAudioProcessor)
};
