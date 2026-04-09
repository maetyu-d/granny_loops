#pragma once

#include "PluginProcessor.h"
#include <juce_gui_extra/juce_gui_extra.h>

class GrannyAudioProcessorEditor final : public juce::AudioProcessorEditor,
                                         private juce::Timer
{
public:
    explicit GrannyAudioProcessorEditor (GrannyAudioProcessor&);
    ~GrannyAudioProcessorEditor() override;

    void paint (juce::Graphics&) override;
    void resized() override;

private:
    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    void timerCallback() override;
    void configureSlider (juce::Slider& slider, juce::Label& label, const juce::String& text);
    void configureCombo (juce::ComboBox& combo, juce::Label& label, const juce::String& text);
    void configureCaption (juce::Label& label, const juce::String& text, juce::Justification justification);

    GrannyAudioProcessor& audioProcessor;

    juce::Label titleLabel;
    juce::Label subtitleLabel;
    juce::Label presetLabel;
    juce::Label modeLabel;
    juce::Label engineLabel;
    juce::Label stretchEngineModeLabel;
    juce::Label monitorModeLabel;
    juce::Label captureSectionLabel;
    juce::Label stretchSectionLabel;
    juce::Label textureSectionLabel;
    juce::Label outputSectionLabel;

    juce::Slider bufferSlider;
    juce::Slider scrubSlider;
    juce::Slider grainSizeSlider;
    juce::Slider densitySlider;
    juce::Slider speedSlider;
    juce::Slider stretchSlider;
    juce::Slider spliceSlider;
    juce::Slider ageSlider;
    juce::Slider blurSlider;
    juce::Slider slipSlider;
    juce::Slider bloomSlider;
    juce::Slider regenSlider;
    juce::Slider mixSlider;
    juce::Slider clockSlider;

    juce::ComboBox directionBox;
    juce::ComboBox pitchModeBox;
    juce::ComboBox presetBox;
    juce::ComboBox stretchEngineBox;
    juce::ComboBox outputMonitorBox;
    juce::ToggleButton freezeButton { "Freeze" };

    juce::Label bufferLabel;
    juce::Label scrubLabel;
    juce::Label grainSizeLabel;
    juce::Label densityLabel;
    juce::Label speedLabel;
    juce::Label stretchLabel;
    juce::Label spliceLabel;
    juce::Label ageLabel;
    juce::Label blurLabel;
    juce::Label slipLabel;
    juce::Label bloomLabel;
    juce::Label regenLabel;
    juce::Label mixLabel;
    juce::Label clockLabel;
    juce::Label directionLabel;
    juce::Label pitchLabel;

    std::unique_ptr<SliderAttachment> bufferAttachment;
    std::unique_ptr<SliderAttachment> scrubAttachment;
    std::unique_ptr<SliderAttachment> grainSizeAttachment;
    std::unique_ptr<SliderAttachment> densityAttachment;
    std::unique_ptr<SliderAttachment> speedAttachment;
    std::unique_ptr<SliderAttachment> stretchAttachment;
    std::unique_ptr<SliderAttachment> spliceAttachment;
    std::unique_ptr<SliderAttachment> ageAttachment;
    std::unique_ptr<SliderAttachment> blurAttachment;
    std::unique_ptr<SliderAttachment> slipAttachment;
    std::unique_ptr<SliderAttachment> bloomAttachment;
    std::unique_ptr<SliderAttachment> regenAttachment;
    std::unique_ptr<SliderAttachment> mixAttachment;
    std::unique_ptr<SliderAttachment> clockAttachment;
    std::unique_ptr<ButtonAttachment> freezeAttachment;
    std::unique_ptr<ComboAttachment> stretchEngineAttachment;
    std::unique_ptr<ComboAttachment> outputMonitorAttachment;
    std::unique_ptr<ComboAttachment> directionAttachment;
    std::unique_ptr<ComboAttachment> pitchAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (GrannyAudioProcessorEditor)
};
