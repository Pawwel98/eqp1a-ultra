#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include "PluginProcessor.h"

class PultecRackLookAndFeel : public juce::LookAndFeel_V4
{
public:
    enum KnobStyle
    {
        StyleFlutedSkirted = 0, // Worn silver-skirted black Pultec Boost/Atten/Bandwidth dial (0..10)
        StyleChickenHead   = 1, // Weathered vintage pointer selector switch (CPS / KCS / ATTEN SEL)
        StyleTrimKnob      = 2  // Industrial weathered trim knob (INPUT / DRIVE / OUTPUT in Tube Bay)
    };

    PultecRackLookAndFeel();

    void drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                          float sliderPosProportional, float rotaryStartAngle,
                          float rotaryEndAngle, juce::Slider& slider) override;

    void drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                          bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    void drawButtonBackground(juce::Graphics& g, juce::Button& button,
                              const juce::Colour& backgroundColour,
                              bool shouldDrawButtonAsHighlighted,
                              bool shouldDrawButtonAsDown) override;

    void drawComboBox(juce::Graphics& g, int width, int height, bool isButtonDown,
                      int buttonX, int buttonY, int buttonW, int buttonH,
                      juce::ComboBox& box) override;
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
    void toggleHudExpanded();
    void refreshPresetComboBox();
    void saveUserPresetDialog();
    void loadUserPresetDialog();

    void generateProceduralTextures();
    void drawScratchedText(juce::Graphics& g, const juce::String& text,
                           int x, int y, int w, int h, float fontSize,
                           juce::Justification just = juce::Justification::centred);
    void drawTubeBayBehindGrille(juce::Graphics& g, juce::Rectangle<float> area);
    void drawEQCurveDisplay(juce::Graphics& g, juce::Rectangle<float> area);
    void drawRackChassis(juce::Graphics& g, juce::Rectangle<float> area);
    void drawRadialDialNumbers(juce::Graphics& g, const juce::Slider& slider,
                               const std::vector<juce::String>& labels,
                               float radiusOffset, float fontSize = 10.5f);
    void updateHoverTooltip();

    PultecEQP1AAudioProcessor& audioProcessor;
    PultecRackLookAndFeel pultecLNF;

    juce::Image weatheredRackImage;
    juce::Image blackGrilleMeshImage;

    bool isHudExpanded = false;
    static constexpr int kWidth         = 960;
    static constexpr int kRackHeight    = 360;
    static constexpr int kHudHeight     = 220;

    juce::TextButton expandHudButton { "EXPAND TUBE BAY" };

    // Top Row: Low Boost, Low Atten, High Boost, High Atten, High Atten Sel
    juce::Slider lowBoostKnob;
    juce::Slider lowAttenKnob;
    juce::Slider highBoostKnob;
    juce::Slider highAttenKnob;
    juce::Slider highAttenFreqSelector;

    // Bottom Row: IN/OUT Toggle, Low Freq (CPS), Bandwidth, High Freq (KCS), OFF/ON Power Switch
    juce::ToggleButton eqInSwitch   { "IN" };
    juce::Slider lowFreqSelector;
    juce::Slider bandwidthKnob;
    juce::Slider highBoostFreqSelector;
    juce::ToggleButton powerSwitch  { "ON" };

    // Expandable Tube Bay Controls
    InputCalibrationSlider inputKnob;
    juce::Slider tubeDriveKnob;
    juce::Slider outputKnob;

    juce::ToggleButton autoGainButton  { "AUTO GAIN" };
    juce::ToggleButton analogHumButton { "60Hz HUM" };

    juce::ComboBox routingBox;
    juce::ComboBox oversamplingBox;
    juce::ComboBox presetBox;
    juce::TextButton savePresetBtn { "SAVE" };
    juce::TextButton loadPresetBtn { "LOAD" };

    std::vector<juce::File> discoveredUserPresets;
    std::unique_ptr<juce::FileChooser> activeFileChooser;

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
