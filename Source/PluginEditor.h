#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

class PultecRackLookAndFeel : public juce::LookAndFeel_V4
{
public:
    enum KnobStyle
    {
        StyleFlutedSkirted = 0, // Classic silver-skirted black Pultec Boost/Atten/Bandwidth dial (0..10)
        StyleChickenHead   = 1, // Vintage pointer selector switch (CPS / KCS / ATTEN SEL)
        StyleTrimKnob      = 2  // Compact utility trim knob (INPUT / DRIVE / OUTPUT)
    };

    PultecRackLookAndFeel();

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider& slider) override;

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                          bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;
};

class InputCalibrationSlider : public juce::Slider
{
public:
    std::function<void()> onShiftClick;

    void mouseDown(const juce::MouseEvent& e) override
    {
        if (e.mods.isShiftDown() && e.mods.isLeftButtonDown() && onShiftClick)
        {
            onShiftClick();
            return;
        }
        juce::Slider::mouseDown(e);
    }
};

class PultecEQP1AAudioProcessorEditor : public juce::AudioProcessorEditor,
                                        private juce::Timer
{
public:
    explicit PultecEQP1AAudioProcessorEditor(PultecEQP1AAudioProcessor&);
    ~PultecEQP1AAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    void drawEQCurveDisplay(juce::Graphics& g, juce::Rectangle<float> area);
    void drawRackChassis(juce::Graphics& g, juce::Rectangle<float> area);
    void updateHoverTooltip();

    PultecEQP1AAudioProcessor& audioProcessor;
    PultecRackLookAndFeel pultecLNF;

    // Main Pultec EQP-1A3 Controls (8 classic controls)
    juce::Slider lowBoostKnob;
    juce::Slider lowAttenKnob;
    juce::Slider lowFreqSelector;

    juce::Slider highBoostKnob;
    juce::Slider bandwidthKnob;
    juce::Slider highBoostFreqSelector;

    juce::Slider highAttenKnob;
    juce::Slider highAttenFreqSelector;

    // Modern Mastering & Analog Drive Controls
    InputCalibrationSlider inputKnob;
    juce::Slider tubeDriveKnob;
    juce::Slider outputKnob;

    juce::ToggleButton eqInButton      { "EQ IN" };
    juce::ToggleButton bypassButton    { "BYPASS" };
    juce::ToggleButton autoGainButton  { "AUTO GAIN" };
    juce::ToggleButton analogHumButton { "60Hz HUM" };

    juce::ComboBox routingBox;
    juce::ComboBox oversamplingBox;
    juce::ComboBox presetBox;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;
    using ComboAttachment  = juce::AudioProcessorValueTreeState::ComboBoxAttachment;

    std::unique_ptr<SliderAttachment> lowBoostAtt, lowAttenAtt, lowFreqAtt;
    std::unique_ptr<SliderAttachment> highBoostAtt, bandwidthAtt, highBoostFreqAtt;
    std::unique_ptr<SliderAttachment> highAttenAtt, highAttenFreqAtt;
    std::unique_ptr<SliderAttachment> inputAtt, tubeDriveAtt, outputAtt;
    std::unique_ptr<ButtonAttachment> eqInAtt, bypassAtt, autoGainAtt, analogHumAtt;
    std::unique_ptr<ComboAttachment>  routingAtt, oversamplingAtt;

    juce::Component* lastHoveredComp = nullptr;
    juce::uint32 hoverStartMs = 0;
    juce::String activeTooltipText;
    bool showCalibrationBanner = false;
    int  calibrationDoneHoldTicks = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PultecEQP1AAudioProcessorEditor)
};
