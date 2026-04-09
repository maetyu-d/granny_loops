#include "PluginEditor.h"

namespace
{
constexpr int knobLabelHeight = 22;
constexpr int textBoxHeight = 22;
const auto white = juce::Colour::fromRGB (245, 245, 245);
const auto whiteDim = juce::Colour::fromRGBA (255, 255, 255, 72);
const auto whiteFaint = juce::Colour::fromRGBA (255, 255, 255, 28);
const auto panelFill = juce::Colour::fromRGB (7, 7, 7);
const auto panelFillAlt = juce::Colour::fromRGB (10, 10, 10);
const auto textDim = juce::Colour::fromRGB (150, 150, 150);

class GrannyLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           const juce::Slider::SliderStyle style, juce::Slider&) override
    {
        juce::ignoreUnused (minSliderPos, maxSliderPos);

        auto bounds = juce::Rectangle<float> ((float) x, (float) y, (float) width, (float) height).reduced (12.0f, 10.0f);

        if (style == juce::Slider::LinearHorizontal)
        {
            auto rail = bounds.withHeight (1.0f).withCentre (bounds.getCentre());
            auto fill = rail.withWidth (juce::jlimit (0.0f, rail.getWidth(), sliderPos - rail.getX()));

            g.setColour (whiteFaint);
            g.fillRect (rail);
            g.setColour (white);
            g.fillRect (fill);

            g.setColour (white);
            g.drawLine (sliderPos, rail.getY() - 7.0f, sliderPos, rail.getBottom() + 7.0f, 1.2f);
        }
    }

    void drawComboBox (juce::Graphics& g, int width, int height, bool,
                       int, int, int, int, juce::ComboBox&) override
    {
        auto bounds = juce::Rectangle<float> (0.0f, 0.0f, (float) width, (float) height);
        g.setColour (panelFill);
        g.fillRect (bounds);
        g.setColour (whiteFaint);
        g.drawRect (bounds, 1.0f);

        juce::Path arrow;
        const auto cx = bounds.getRight() - 22.0f;
        const auto cy = bounds.getCentreY();
        arrow.startNewSubPath (cx - 7.0f, cy - 3.0f);
        arrow.lineTo (cx, cy + 4.0f);
        arrow.lineTo (cx + 7.0f, cy - 3.0f);
        g.setColour (white);
        g.strokePath (arrow, juce::PathStrokeType (1.7f, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    void drawToggleButton (juce::Graphics& g, juce::ToggleButton& button, bool, bool) override
    {
        auto bounds = button.getLocalBounds().toFloat();
        auto led = bounds.removeFromLeft (34.0f).withSizeKeepingCentre (18.0f, 18.0f);
        bounds.removeFromLeft (8.0f);

        g.setColour (button.getToggleState() ? white : panelFill);
        g.fillRect (led);
        g.setColour (button.getToggleState() ? white : whiteFaint);
        g.drawRect (led, 1.0f);

        g.setColour (white);
        g.setFont (juce::Font (juce::FontOptions (15.0f).withStyle ("Bold")));
        g.drawFittedText (button.getButtonText(), bounds.toNearestInt(), juce::Justification::centredLeft, 1);
    }

    juce::Font getComboBoxFont (juce::ComboBox&) override
    {
        return juce::Font (juce::FontOptions (15.0f).withStyle ("Bold"));
    }
};
}

GrannyAudioProcessorEditor::GrannyAudioProcessorEditor (GrannyAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    static GrannyLookAndFeel lookAndFeel;
    setLookAndFeel (&lookAndFeel);

    titleLabel.setText ("GRANNY", juce::dontSendNotification);
    titleLabel.setFont (juce::Font (juce::FontOptions (34.0f).withStyle ("Bold")));
    titleLabel.setColour (juce::Label::textColourId, white);
    addAndMakeVisible (titleLabel);

    subtitleLabel.setText ("GRANULAR DELAY / FREEZE SAMPLER / MICROLOOP UNIT", juce::dontSendNotification);
    subtitleLabel.setFont (juce::Font (juce::FontOptions (13.0f).withStyle ("Bold")));
    subtitleLabel.setColour (juce::Label::textColourId, textDim);
    addAndMakeVisible (subtitleLabel);

    configureCaption (presetLabel, "PROGRAM", juce::Justification::centredLeft);
    configureCaption (modeLabel, "CONTROL", juce::Justification::centredLeft);
    configureCaption (engineLabel, "ENGINE", juce::Justification::centredLeft);
    configureCaption (captureSectionLabel, "CAPTURE / MOTION", juce::Justification::centredLeft);
    configureCaption (stretchSectionLabel, "TEXTURE / OUTPUT", juce::Justification::centredLeft);
    configureCaption (textureSectionLabel, "TEXTURE", juce::Justification::centredLeft);
    configureCaption (outputSectionLabel, "OUTPUT", juce::Justification::centredLeft);

    configureSlider (bufferSlider, bufferLabel, "BUFFER");
    configureSlider (scrubSlider, scrubLabel, "SCRUB");
    configureSlider (grainSizeSlider, grainSizeLabel, "SIZE");
    configureSlider (densitySlider, densityLabel, "LAUNCH");
    configureSlider (speedSlider, speedLabel, "SPEED");
    configureSlider (stretchSlider, stretchLabel, "STRETCH");
    configureSlider (spliceSlider, spliceLabel, "SPLICE");
    configureSlider (ageSlider, ageLabel, "AGE");
    configureSlider (blurSlider, blurLabel, "BLUR");
    configureSlider (slipSlider, slipLabel, "SLIP");
    configureSlider (bloomSlider, bloomLabel, "BLOOM");
    configureSlider (regenSlider, regenLabel, "REGEN");
    configureSlider (mixSlider, mixLabel, "MIX");
    configureSlider (clockSlider, clockLabel, "CLOCK");

    directionBox.addItemList ({ "Forward", "Reverse", "Random reverse", "Splice jump" }, 1);
    pitchModeBox.addItemList ({ "Original", "Random +12", "Random +/-12" }, 1);
    stretchEngineBox.addItemList ({ "Hybrid", "Spectral" }, 1);
    outputMonitorBox.addItemList ({ "Full", "Errors Only" }, 1);
    configureCombo (stretchEngineBox, stretchEngineModeLabel, "ENGINE");
    configureCombo (outputMonitorBox, monitorModeLabel, "MONITOR");
    configureCombo (directionBox, directionLabel, "DIRECTION");
    configureCombo (pitchModeBox, pitchLabel, "PITCH");

    presetBox.addItemList (GrannyAudioProcessor::getPresetNames(), 1);
    presetBox.setJustificationType (juce::Justification::centredLeft);
    presetBox.onChange = [this]
    {
        const auto selected = presetBox.getSelectedItemIndex();
        if (selected >= 0)
            audioProcessor.applyPreset (selected);
    };
    presetBox.setSelectedItemIndex (audioProcessor.getCurrentProgram(), juce::dontSendNotification);
    addAndMakeVisible (presetBox);

    freezeButton.setButtonText ("FREEZE");
    addAndMakeVisible (freezeButton);

    auto& state = audioProcessor.parameters;
    bufferAttachment = std::make_unique<SliderAttachment> (state, "bufferSeconds", bufferSlider);
    scrubAttachment = std::make_unique<SliderAttachment> (state, "scrub", scrubSlider);
    grainSizeAttachment = std::make_unique<SliderAttachment> (state, "grainSize", grainSizeSlider);
    densityAttachment = std::make_unique<SliderAttachment> (state, "density", densitySlider);
    speedAttachment = std::make_unique<SliderAttachment> (state, "speed", speedSlider);
    stretchAttachment = std::make_unique<SliderAttachment> (state, "stretch", stretchSlider);
    spliceAttachment = std::make_unique<SliderAttachment> (state, "splice", spliceSlider);
    ageAttachment = std::make_unique<SliderAttachment> (state, "age", ageSlider);
    blurAttachment = std::make_unique<SliderAttachment> (state, "blur", blurSlider);
    slipAttachment = std::make_unique<SliderAttachment> (state, "slip", slipSlider);
    bloomAttachment = std::make_unique<SliderAttachment> (state, "bloom", bloomSlider);
    regenAttachment = std::make_unique<SliderAttachment> (state, "regen", regenSlider);
    mixAttachment = std::make_unique<SliderAttachment> (state, "mix", mixSlider);
    clockAttachment = std::make_unique<SliderAttachment> (state, "clock", clockSlider);
    freezeAttachment = std::make_unique<ButtonAttachment> (state, "freeze", freezeButton);
    stretchEngineAttachment = std::make_unique<ComboAttachment> (state, "stretchEngine", stretchEngineBox);
    outputMonitorAttachment = std::make_unique<ComboAttachment> (state, "monitor", outputMonitorBox);
    directionAttachment = std::make_unique<ComboAttachment> (state, "direction", directionBox);
    pitchAttachment = std::make_unique<ComboAttachment> (state, "pitchMode", pitchModeBox);

    setSize (1120, 620);
    startTimerHz (8);
}

GrannyAudioProcessorEditor::~GrannyAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel (nullptr);
}

void GrannyAudioProcessorEditor::timerCallback()
{
    presetBox.setSelectedItemIndex (audioProcessor.getCurrentProgram(), juce::dontSendNotification);
}

void GrannyAudioProcessorEditor::configureCaption (juce::Label& label, const juce::String& text, juce::Justification justification)
{
    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (justification);
    label.setFont (juce::Font (juce::FontOptions (12.0f).withStyle ("Bold")));
    label.setColour (juce::Label::textColourId, textDim);
    addAndMakeVisible (label);
}

void GrannyAudioProcessorEditor::configureSlider (juce::Slider& slider, juce::Label& label, const juce::String& text)
{
    slider.setSliderStyle (juce::Slider::LinearHorizontal);
    slider.setTextBoxStyle (juce::Slider::TextBoxRight, false, 88, textBoxHeight);
    slider.setColour (juce::Slider::textBoxTextColourId, white);
    slider.setColour (juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    slider.setColour (juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);
    addAndMakeVisible (slider);

    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centredLeft);
    label.setFont (juce::Font (juce::FontOptions (14.0f).withStyle ("Bold")));
    label.setColour (juce::Label::textColourId, white);
    addAndMakeVisible (label);
}

void GrannyAudioProcessorEditor::configureCombo (juce::ComboBox& combo, juce::Label& label, const juce::String& text)
{
    addAndMakeVisible (combo);
    label.setText (text, juce::dontSendNotification);
    label.setJustificationType (juce::Justification::centredLeft);
    label.setFont (juce::Font (juce::FontOptions (12.0f).withStyle ("Bold")));
    label.setColour (juce::Label::textColourId, textDim);
    addAndMakeVisible (label);
}

void GrannyAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (panelFillAlt);

    auto bounds = getLocalBounds().toFloat();
    g.setColour (panelFillAlt);
    g.fillRect (bounds);

    auto body = getLocalBounds().reduced (24);
    g.setColour (panelFill);
    g.fillRect (body);
    g.setColour (white);
    g.drawRect (body, 1);

    auto content = body.reduced (10);
    auto utility = content.removeFromTop (58);
    content.removeFromTop (10);
    auto columns = content;

    auto drawPanel = [&g] (juce::Rectangle<int> area, float radius)
    {
        g.setColour (juce::Colour::fromRGBA (255, 255, 255, 8));
        g.fillRoundedRectangle (area.toFloat(), radius);
        g.setColour (whiteFaint);
        g.drawRoundedRectangle (area.toFloat(), radius, 0.8f);
    };

    drawPanel (utility, 8.0f);

    const int columnGap = 16;
    auto leftColumn = columns.removeFromLeft ((columns.getWidth() - columnGap) / 2);
    columns.removeFromLeft (columnGap);
    auto rightColumn = columns;

    drawPanel (leftColumn, 10.0f);
    drawPanel (rightColumn, 10.0f);
}

void GrannyAudioProcessorEditor::resized()
{
    auto body = getLocalBounds().reduced (24);

    auto bodyArea = body.reduced (10);
    auto utilityArea = bodyArea.removeFromTop (58).reduced (10);
    bodyArea.removeFromTop (10);

    titleLabel.setBounds (0, 0, 0, 0);
    subtitleLabel.setBounds (0, 0, 0, 0);
    engineLabel.setBounds (0, 0, 0, 0);
    stretchSectionLabel.setBounds (0, 0, 0, 0);
    outputSectionLabel.setBounds (0, 0, 0, 0);

    utilityArea.removeFromTop (10);

    presetLabel.setBounds (0, 0, 0, 0);
    modeLabel.setBounds (0, 0, 0, 0);
    stretchEngineModeLabel.setBounds (0, 0, 0, 0);
    monitorModeLabel.setBounds (0, 0, 0, 0);
    directionLabel.setBounds (0, 0, 0, 0);
    pitchLabel.setBounds (0, 0, 0, 0);

    auto utilityRow = utilityArea.removeFromTop (36);
    const int gap = 12;
    const int presetCell = 280;
    const int freezeCell = 140;
    const int smallCell = (utilityRow.getWidth() - presetCell - freezeCell - gap * 5) / 4;

    presetBox.setBounds (utilityRow.removeFromLeft (presetCell));
    utilityRow.removeFromLeft (gap);
    freezeButton.setBounds (utilityRow.removeFromLeft (freezeCell).reduced (0, 1));
    utilityRow.removeFromLeft (gap);
    stretchEngineBox.setBounds (utilityRow.removeFromLeft (smallCell).reduced (0, 1));
    utilityRow.removeFromLeft (gap);
    outputMonitorBox.setBounds (utilityRow.removeFromLeft (smallCell).reduced (0, 1));
    utilityRow.removeFromLeft (gap);
    directionBox.setBounds (utilityRow.removeFromLeft (smallCell).reduced (0, 1));
    utilityRow.removeFromLeft (gap);
    pitchModeBox.setBounds (utilityRow.removeFromLeft (smallCell).reduced (0, 1));

    auto columnsArea = bodyArea;
    const int columnGap = 16;
    auto leftColumnBounds = columnsArea.removeFromLeft ((columnsArea.getWidth() - columnGap) / 2);
    columnsArea.removeFromLeft (columnGap);
    auto rightColumnBounds = columnsArea;

    auto leftColumn = leftColumnBounds.reduced (20, 16);
    auto rightColumn = rightColumnBounds.reduced (20, 16);

    auto placeSliderStack = [] (juce::Rectangle<int> area, std::initializer_list<std::pair<juce::Label*, juce::Slider*>> items)
    {
        constexpr int rowHeight = 50;
        for (const auto& [label, slider] : items)
        {
            auto row = area.removeFromTop (rowHeight);
            label->setBounds (row.removeFromTop (knobLabelHeight));
            row.removeFromTop (3);
            slider->setBounds (row);
            area.removeFromTop (10);
        }
    };

    placeSliderStack (leftColumn, {
        { &bufferLabel, &bufferSlider },
        { &scrubLabel, &scrubSlider },
        { &grainSizeLabel, &grainSizeSlider },
        { &densityLabel, &densitySlider },
        { &speedLabel, &speedSlider },
        { &stretchLabel, &stretchSlider },
        { &spliceLabel, &spliceSlider }
    });

    placeSliderStack (rightColumn, {
        { &clockLabel, &clockSlider },
        { &mixLabel, &mixSlider },
        { &ageLabel, &ageSlider },
        { &blurLabel, &blurSlider },
        { &slipLabel, &slipSlider },
        { &bloomLabel, &bloomSlider },
        { &regenLabel, &regenSlider }
    });

    captureSectionLabel.setBounds (0, 0, 0, 0);
    textureSectionLabel.setBounds (0, 0, 0, 0);
}
