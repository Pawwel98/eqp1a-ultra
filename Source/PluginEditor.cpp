#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
PultecRackLookAndFeel::PultecRackLookAndFeel()
{
    setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff141c24));
    setColour(juce::ComboBox::textColourId,       juce::Colour(0xffece6d8));
    setColour(juce::ComboBox::outlineColourId,    juce::Colour(0xff4a5d70));
    setColour(juce::PopupMenu::backgroundColourId,juce::Colour(0xff141c24));
    setColour(juce::PopupMenu::textColourId,      juce::Colour(0xffece6d8));
    setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xff2c4d6e));
}

void PultecRackLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                             float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                             juce::Slider& slider)
{
    const auto bounds = juce::Rectangle<float>((float)x, (float)y, (float)width, (float)height).reduced(2.0f);
    const float cx = bounds.getCentreX();
    const float cy = bounds.getCentreY();
    const float radius = std::min(bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    const int style = slider.getProperties().getWithDefault("knobStyle", (int)StyleFlutedSkirted);

    if (style == StyleFlutedSkirted || style == StyleTrimKnob)
    {
        // Soft drop shadow on the blue Pultec faceplate
        g.setColour(juce::Colours::black.withAlpha(0.58f));
        g.fillEllipse(cx - radius * 0.76f + 3.0f, cy - radius * 0.76f + 5.0f, radius * 1.52f, radius * 1.52f);

        // Outer black/silver beveled skirt
        const float skirtR = radius * 0.76f;
        juce::ColourGradient skirtGrad(juce::Colour(0xff525860), cx - skirtR, cy - skirtR,
                                       juce::Colour(0xff121518), cx + skirtR, cy + skirtR, false);
        skirtGrad.addColour(0.45, juce::Colour(0xff2a2e33));
        g.setGradientFill(skirtGrad);
        g.fillEllipse(cx - skirtR, cy - skirtR, skirtR * 2.0f, skirtR * 2.0f);

        // Silver machined rim ring (just like original Davies/Daka-Ware Pultec knobs)
        g.setColour(juce::Colour(0xffb8c0c8).withAlpha(0.65f));
        g.drawEllipse(cx - skirtR, cy - skirtR, skirtR * 2.0f, skirtR * 2.0f, 1.3f);

        // Fluted black bakelite body (10 scalloped flutes)
        const float capR = radius * 0.62f;
        juce::ColourGradient capGrad(juce::Colour(0xff3a3f46), cx - capR * 0.6f, cy - capR * 0.6f,
                                     juce::Colour(0xff0d0f12), cx + capR * 0.7f, cy + capR * 0.7f, false);
        g.setGradientFill(capGrad);
        g.fillEllipse(cx - capR, cy - capR, capR * 2.0f, capR * 2.0f);

        // Scalloped grip flutes around the circumference
        for (int r = 0; r < 10; ++r)
        {
            const float ribA = angle + (float)r * (juce::MathConstants<float>::twoPi / 10.0f) - juce::MathConstants<float>::halfPi;
            const float fx = cx + std::cos(ribA) * (capR * 0.96f);
            const float fy = cy + std::sin(ribA) * (capR * 0.96f);
            g.setColour(juce::Colour(0xff08090c).withAlpha(0.85f));
            g.fillEllipse(fx - 3.6f, fy - 3.6f, 7.2f, 7.2f);
            g.setColour(juce::Colour(0xff68707a).withAlpha(0.35f));
            g.drawEllipse(fx - 3.6f, fy - 3.6f, 7.2f, 7.2f, 0.8f);
        }

        // Inner concave bakelite center cap
        const float innerR = capR * 0.58f;
        juce::ColourGradient innerGrad(juce::Colour(0xff14171a), cx - innerR, cy - innerR,
                                       juce::Colour(0xff2c3036), cx + innerR, cy + innerR, false);
        g.setGradientFill(innerGrad);
        g.fillEllipse(cx - innerR, cy - innerR, innerR * 2.0f, innerR * 2.0f);
        g.setColour(juce::Colour(0xff4a5058).withAlpha(0.6f));
        g.drawEllipse(cx - innerR, cy - innerR, innerR * 2.0f, innerR * 2.0f, 1.1f);

        // White engraved indicator line on the outer skirt
        const float indA = angle - juce::MathConstants<float>::halfPi;
        g.setColour(juce::Colour(0xfff5efe2));
        g.drawLine(cx + std::cos(indA) * (innerR * 0.95f), cy + std::sin(indA) * (innerR * 0.95f),
                   cx + std::cos(indA) * (skirtR * 0.98f), cy + std::sin(indA) * (skirtR * 0.98f), 2.5f);
    }
    else // StyleChickenHead (Authentic Pultec black bakelite pointer switch for CPS / KCS / ATTEN SEL)
    {
        const float baseR = radius * 0.44f;
        g.setColour(juce::Colours::black.withAlpha(0.55f));
        g.fillEllipse(cx - baseR + 3.0f, cy - baseR + 5.0f, baseR * 2.0f, baseR * 2.0f);

        // Chrome hex bushing nut under switch
        g.setColour(juce::Colour(0xffa4aeb8));
        g.fillEllipse(cx - baseR * 0.72f, cy - baseR * 0.72f, baseR * 1.44f, baseR * 1.44f);

        // Vintage tapered bar/pointer body (matches CPS / KCS / ATTEN SEL on hardware photo)
        juce::Path pointer;
        pointer.startNewSubPath(-baseR * 0.38f,  baseR * 0.68f);
        pointer.lineTo         ( baseR * 0.38f,  baseR * 0.68f);
        pointer.lineTo         ( baseR * 0.22f, -radius * 0.76f);
        pointer.lineTo         (-baseR * 0.22f, -radius * 0.76f);
        pointer.closeSubPath();

        // Drop shadow of pointer bar
        g.setColour(juce::Colours::black.withAlpha(0.50f));
        g.fillPath(pointer, juce::AffineTransform::rotation(angle).translated(cx + 3.0f, cy + 5.0f));

        // Glossy black bakelite pointer body
        juce::ColourGradient ptrGrad(juce::Colour(0xff32373d), cx - baseR, cy - baseR,
                                     juce::Colour(0xff0c0e10), cx + baseR, cy + baseR, false);
        g.setGradientFill(ptrGrad);
        g.fillPath(pointer, juce::AffineTransform::rotation(angle).translated(cx, cy));

        g.setColour(juce::Colour(0xff5a626b));
        g.strokePath(pointer, juce::PathStrokeType(1.2f), juce::AffineTransform::rotation(angle).translated(cx, cy));

        // Center round turret cap
        g.setColour(juce::Colour(0xff181b1f));
        g.fillEllipse(cx - baseR * 0.56f, cy - baseR * 0.56f, baseR * 1.12f, baseR * 1.12f);
        g.setColour(juce::Colour(0xff4e565e));
        g.drawEllipse(cx - baseR * 0.56f, cy - baseR * 0.56f, baseR * 1.12f, baseR * 1.12f, 1.0f);

        // Crisp white pointer line on tip
        const float indA = angle - juce::MathConstants<float>::halfPi;
        g.setColour(juce::Colour(0xfff5efe2));
        g.drawLine(cx + std::cos(indA) * (baseR * 0.30f), cy + std::sin(indA) * (baseR * 0.30f),
                   cx + std::cos(indA) * (radius * 0.74f), cy + std::sin(indA) * (radius * 0.74f), 2.2f);
    }
}

void PultecRackLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                                             bool isHighlighted, bool)
{
    const auto name = button.getButtonText();
    auto r = button.getLocalBounds().toFloat();

    // 1. Hardware Left Chrome Bat Toggle Switch ("IN")
    if (name == "IN")
    {
        const float cx = r.getCentreX();
        const float cy = r.getCentreY() + 4.0f;
        const bool eqIn = button.getToggleState();

        // Chrome hex washer & bushing
        g.setColour(juce::Colours::black.withAlpha(0.5f));
        g.fillEllipse(cx - 14.0f, cy - 12.0f, 28.0f, 28.0f);

        juce::ColourGradient nutGrad(juce::Colour(0xffe4e8ec), cx - 12.0f, cy - 12.0f,
                                     juce::Colour(0xff687078), cx + 12.0f, cy + 12.0f, false);
        g.setGradientFill(nutGrad);
        g.fillEllipse(cx - 13.0f, cy - 13.0f, 26.0f, 26.0f);
        g.setColour(juce::Colour(0xff252a30));
        g.drawEllipse(cx - 13.0f, cy - 13.0f, 26.0f, 26.0f, 1.3f);

        g.setColour(juce::Colour(0xff181c20));
        g.fillEllipse(cx - 6.5f, cy - 6.5f, 13.0f, 13.0f);

        // Chrome bat lever pointing UP when IN, DOWN when OUT
        const float tipY = eqIn ? (cy - 18.0f) : (cy + 18.0f);
        g.setColour(juce::Colours::black.withAlpha(0.45f));
        g.drawLine(cx + 2.0f, cy + 2.0f, cx + 2.0f, tipY + 3.0f, 7.5f);

        g.setColour(juce::Colour(0xffd8dee4));
        g.drawLine(cx, cy, cx, tipY, 6.5f);
        g.setColour(juce::Colours::white);
        g.fillEllipse(cx - 4.5f, tipY - 4.5f, 9.0f, 9.0f);
        return;
    }

    // 2. Hardware Right OFF / ON Rotary Switch ("ON" -> attached to "bypass" parameter)
    if (name == "ON")
    {
        const float cx = r.getCentreX();
        const float cy = r.getCentreY() + 6.0f;
        const bool isBypassed = button.getToggleState(); // true = OFF (Bypassed), false = ON (Active)
        const float angle = isBypassed ? -0.48f : 0.48f;

        const float knobR = 20.0f;
        g.setColour(juce::Colours::black.withAlpha(0.55f));
        g.fillEllipse(cx - knobR + 2.5f, cy - knobR + 4.0f, knobR * 2.0f, knobR * 2.0f);

        juce::ColourGradient bodyGrad(juce::Colour(0xff32373d), cx - knobR, cy - knobR,
                                      juce::Colour(0xff0d0f12), cx + knobR, cy + knobR, false);
        g.setGradientFill(bodyGrad);
        g.fillEllipse(cx - knobR, cy - knobR, knobR * 2.0f, knobR * 2.0f);
        g.setColour(juce::Colour(0xff565e66));
        g.drawEllipse(cx - knobR, cy - knobR, knobR * 2.0f, knobR * 2.0f, 1.2f);

        // Pointer tip
        juce::Path tip;
        tip.addTriangle(-5.0f, 4.0f, 5.0f, 4.0f, 0.0f, -knobR * 1.18f);
        g.setColour(juce::Colour(0xff1a1d22));
        g.fillPath(tip, juce::AffineTransform::rotation(angle).translated(cx, cy));
        g.setColour(juce::Colour(0xfff5efe2));
        const float indA = angle - juce::MathConstants<float>::halfPi;
        g.drawLine(cx + std::cos(indA) * 5.0f, cy + std::sin(indA) * 5.0f,
                   cx + std::cos(indA) * (knobR * 1.05f), cy + std::sin(indA) * (knobR * 1.05f), 2.0f);
        return;
    }

    // 3. Standard HUD Toggle Button (AUTO GAIN / 60Hz HUM)
    auto box = r.reduced(2.0f);
    const bool on = button.getToggleState();
    juce::Colour bg = on ? juce::Colour(0xff2b6496) : juce::Colour(0xff18222c);
    if (isHighlighted) bg = bg.brighter(0.12f);

    g.setColour(bg);
    g.fillRoundedRectangle(box, 5.0f);
    g.setColour(juce::Colour(0xff6b849c).withAlpha(0.75f));
    g.drawRoundedRectangle(box, 5.0f, 1.1f);

    g.setColour(on ? juce::Colour(0xffffdd66) : juce::Colour(0xff42505e));
    g.fillEllipse(box.getX() + 7.0f, box.getCentreY() - 3.5f, 7.0f, 7.0f);

    g.setColour(juce::Colour(0xfff5efe2));
    g.setFont(juce::Font(11.0f, juce::Font::bold));
    g.drawText(name, box.withTrimmedLeft(16.0f), juce::Justification::centred, false);
}

void PultecRackLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                                 const juce::Colour&, bool isHighlighted, bool isDown)
{
    auto r = button.getLocalBounds().toFloat().reduced(1.5f);
    juce::Colour bg = juce::Colour(0xff1c2b3a);
    if (button.getButtonText().contains("HUD") || button.getButtonText().contains("РАСШИРИТЬ") || button.getButtonText().contains("СКРЫТЬ"))
        bg = juce::Colour(0xff233a4e);
    if (isHighlighted) bg = bg.brighter(0.15f);
    if (isDown)        bg = bg.darker(0.15f);

    g.setColour(bg);
    g.fillRoundedRectangle(r, 5.0f);
    g.setColour(juce::Colour(0xff7fa0be).withAlpha(0.75f));
    g.drawRoundedRectangle(r, 5.0f, 1.2f);
}

//==============================================================================
PultecEQP1AAudioProcessorEditor::PultecEQP1AAudioProcessorEditor(PultecEQP1AAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setLookAndFeel(&pultecLNF);

    auto configDial = [this](juce::Slider& s, int style, double defVal, float startRad, float endRad)
    {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        s.getProperties().set("knobStyle", style);
        s.setRotaryParameters(startRad, endRad, true);
        s.setDoubleClickReturnValue(true, defVal);
        addAndMakeVisible(s);
    };

    const float stdStart = juce::MathConstants<float>::pi * 1.22f;
    const float stdEnd   = juce::MathConstants<float>::pi * 2.78f;

    // 5 Fluted Skirted Knobs (0..10 scale)
    configDial(lowBoostKnob,  PultecRackLookAndFeel::StyleFlutedSkirted, 0.0, stdStart, stdEnd);
    configDial(lowAttenKnob,  PultecRackLookAndFeel::StyleFlutedSkirted, 0.0, stdStart, stdEnd);
    configDial(highBoostKnob, PultecRackLookAndFeel::StyleFlutedSkirted, 0.0, stdStart, stdEnd);
    configDial(highAttenKnob, PultecRackLookAndFeel::StyleFlutedSkirted, 0.0, stdStart, stdEnd);
    configDial(bandwidthKnob, PultecRackLookAndFeel::StyleFlutedSkirted, 5.0, stdStart, stdEnd);

    // 3 Vintage Pointer Selector Switches matching angles on image_f1567c.png
    // CPS: 20, 30, 60, 100 (top arc from -52 deg to +52 deg)
    configDial(lowFreqSelector, PultecRackLookAndFeel::StyleChickenHead, 1.0,
               juce::MathConstants<float>::twoPi - 0.90f,
               juce::MathConstants<float>::twoPi + 0.90f);

    // KCS: 3, 4, 5, 8, 10, 12, 16 (wide top arc from -90 deg to +90 deg)
    configDial(highBoostFreqSelector, PultecRackLookAndFeel::StyleChickenHead, 5.0,
               juce::MathConstants<float>::twoPi - 1.52f,
               juce::MathConstants<float>::twoPi + 1.52f);

    // ATTEN SEL: 5, 10, 20 (compact top arc from -36 deg to +36 deg)
    configDial(highAttenFreqSelector, PultecRackLookAndFeel::StyleChickenHead, 2.0,
               juce::MathConstants<float>::twoPi - 0.62f,
               juce::MathConstants<float>::twoPi + 0.62f);

    // Top HUD Trim Knobs
    configDial(inputKnob,     PultecRackLookAndFeel::StyleTrimKnob, 0.0, stdStart, stdEnd);
    configDial(tubeDriveKnob, PultecRackLookAndFeel::StyleTrimKnob, 2.5, stdStart, stdEnd);
    configDial(outputKnob,    PultecRackLookAndFeel::StyleTrimKnob, 0.0, stdStart, stdEnd);

    inputKnob.onShiftClick = [this]()
    {
        audioProcessor.dspEngine.startInputCalibration();
        showCalibrationBanner = true;
        calibrationDoneHoldTicks = 0;
        repaint();
    };

    addAndMakeVisible(eqInSwitch);
    addAndMakeVisible(powerSwitch);
    addAndMakeVisible(autoGainButton);
    addAndMakeVisible(analogHumButton);

    expandHudButton.setButtonText("▲ РАСШИРИТЬ / ANALYZER & PRESETS");
    expandHudButton.onClick = [this]() { toggleHudExpanded(); };
    addAndMakeVisible(expandHudButton);

    savePresetBtn.onClick = [this]() { saveUserPresetDialog(); };
    loadPresetBtn.onClick = [this]() { loadUserPresetDialog(); };
    addAndMakeVisible(savePresetBtn);
    addAndMakeVisible(loadPresetBtn);

    routingBox.addItemList({ "STEREO (L/R)", "MID ONLY (Center)", "SIDE ONLY (Width)", "DUAL MONO (Analog)" }, 1);
    addAndMakeVisible(routingBox);

    oversamplingBox.addItemList({ "1x (Zero Latency)", "2x (Studio HD)", "4x (Mastering Ultra)" }, 1);
    addAndMakeVisible(oversamplingBox);

    refreshPresetComboBox();
    presetBox.setSelectedId(1, juce::dontSendNotification);
    presetBox.onChange = [this]()
    {
        const int selId = presetBox.getSelectedId();
        const int numFactory = audioProcessor.getNumPrograms();
        if (selId >= 1 && selId <= numFactory)
        {
            audioProcessor.applyFactoryPreset(selId - 1);
        }
        else if (selId >= 100)
        {
            const int userIdx = selId - 100;
            if (userIdx >= 0 && userIdx < (int)discoveredUserPresets.size())
                audioProcessor.loadUserPresetFromFile(discoveredUserPresets[(size_t)userIdx]);
        }
        repaint();
    };
    addAndMakeVisible(presetBox);

    auto& apvts = audioProcessor.apvts;
    lowBoostAtt      = std::make_unique<SliderAttachment>(apvts, "low_boost",       lowBoostKnob);
    lowAttenAtt      = std::make_unique<SliderAttachment>(apvts, "low_atten",       lowAttenKnob);
    lowFreqAtt       = std::make_unique<SliderAttachment>(apvts, "low_freq",        lowFreqSelector);

    highBoostAtt     = std::make_unique<SliderAttachment>(apvts, "high_boost",      highBoostKnob);
    bandwidthAtt     = std::make_unique<SliderAttachment>(apvts, "bandwidth",       bandwidthKnob);
    highBoostFreqAtt = std::make_unique<SliderAttachment>(apvts, "high_boost_freq", highBoostFreqSelector);

    highAttenAtt     = std::make_unique<SliderAttachment>(apvts, "high_atten",      highAttenKnob);
    highAttenFreqAtt = std::make_unique<SliderAttachment>(apvts, "high_atten_freq", highAttenFreqSelector);

    inputAtt         = std::make_unique<SliderAttachment>(apvts, "input_db",        inputKnob);
    tubeDriveAtt     = std::make_unique<SliderAttachment>(apvts, "tube_drive_db",   tubeDriveKnob);
    outputAtt        = std::make_unique<SliderAttachment>(apvts, "output_db",       outputKnob);

    eqInAtt          = std::make_unique<ButtonAttachment>(apvts, "eq_in",           eqInSwitch);
    bypassAtt        = std::make_unique<ButtonAttachment>(apvts, "bypass",          powerSwitch);
    autoGainAtt      = std::make_unique<ButtonAttachment>(apvts, "auto_gain",       autoGainButton);
    analogHumAtt     = std::make_unique<ButtonAttachment>(apvts, "analog_hum",      analogHumButton);

    routingAtt       = std::make_unique<ComboAttachment>(apvts, "routing_mode",     routingBox);
    oversamplingAtt  = std::make_unique<ComboAttachment>(apvts, "oversampling",     oversamplingBox);

    // Start in Closed / Authentic Hardware View by default (960 x 360)
    isHudExpanded = false;
    setSize(kWidth, kRackHeight);

    startTimerHz(30);
}

PultecEQP1AAudioProcessorEditor::~PultecEQP1AAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void PultecEQP1AAudioProcessorEditor::toggleHudExpanded()
{
    isHudExpanded = !isHudExpanded;
    expandHudButton.setButtonText(isHudExpanded
        ? "▼ СКРЫТЬ HUD / COLLAPSE"
        : "▲ РАСШИРИТЬ / ANALYZER & PRESETS");
    setSize(kWidth, isHudExpanded ? (kRackHeight + kHudHeight) : kRackHeight);
    resized();
    repaint();
}

void PultecEQP1AAudioProcessorEditor::refreshPresetComboBox()
{
    const int prevId = presetBox.getSelectedId();
    presetBox.clear(juce::dontSendNotification);

    for (int i = 0; i < audioProcessor.getNumPrograms(); ++i)
        presetBox.addItem(audioProcessor.getProgramName(i), i + 1);

    discoveredUserPresets.clear();
    auto dir = audioProcessor.getUserPresetsFolder();
    juce::Array<juce::File> userFiles;
    dir.findChildFiles(userFiles, juce::File::findFiles, false, "*.pultec;*.xml");
    if (!userFiles.isEmpty())
    {
        presetBox.addSeparator();
        for (int i = 0; i < userFiles.size(); ++i)
        {
            discoveredUserPresets.push_back(userFiles[i]);
            presetBox.addItem("[USER] " + userFiles[i].getFileNameWithoutExtension(), 100 + i);
        }
    }

    if (prevId > 0)
        presetBox.setSelectedId(prevId, juce::dontSendNotification);
}

void PultecEQP1AAudioProcessorEditor::saveUserPresetDialog()
{
    auto startDir = audioProcessor.getUserPresetsFolder().getChildFile("My_Pultec_Preset.pultec");
    activeFileChooser = std::make_unique<juce::FileChooser>(
        "Save Pultec EQP-1A Preset", startDir, "*.pultec;*.xml");

    auto flags = juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
               | juce::FileBrowserComponent::warnAboutOverwriting;

    activeFileChooser->launchAsync(flags, [this](const juce::FileChooser& fc)
    {
        auto file = fc.getResult();
        if (file != juce::File{})
        {
            if (!file.hasFileExtension(".pultec") && !file.hasFileExtension(".xml"))
                file = file.withFileExtension(".pultec");

            if (audioProcessor.saveUserPresetToFile(file))
            {
                refreshPresetComboBox();
                for (size_t i = 0; i < discoveredUserPresets.size(); ++i)
                {
                    if (discoveredUserPresets[i] == file)
                    {
                        presetBox.setSelectedId(100 + (int)i, juce::dontSendNotification);
                        break;
                    }
                }
            }
        }
    });
}

void PultecEQP1AAudioProcessorEditor::loadUserPresetDialog()
{
    auto startDir = audioProcessor.getUserPresetsFolder();
    activeFileChooser = std::make_unique<juce::FileChooser>(
        "Load Pultec EQP-1A Preset", startDir, "*.pultec;*.xml");

    auto flags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

    activeFileChooser->launchAsync(flags, [this](const juce::FileChooser& fc)
    {
        auto file = fc.getResult();
        if (file.existsAsFile())
        {
            if (audioProcessor.loadUserPresetFromFile(file))
            {
                refreshPresetComboBox();
                repaint();
            }
        }
    });
}

void PultecEQP1AAudioProcessorEditor::timerCallback()
{
    if (showCalibrationBanner && !audioProcessor.dspEngine.isCalibrationActive())
    {
        if (++calibrationDoneHoldTicks > 55)
            showCalibrationBanner = false;
    }

    updateHoverTooltip();
    repaint();
}

void PultecEQP1AAudioProcessorEditor::updateHoverTooltip()
{
    const auto mousePos = getMouseXYRelative();
    juce::Component* comp = getComponentAt(mousePos);

    if (juce::ModifierKeys::currentModifiers.isAnyMouseButtonDown())
    {
        activeTooltipText.clear();
        return;
    }

    if (comp != lastHoveredComp)
    {
        lastHoveredComp = comp;
        hoverStartMs = juce::Time::getMillisecondCounter();
        activeTooltipText.clear();
        return;
    }

    if (comp != nullptr && (juce::Time::getMillisecondCounter() - hoverStartMs) >= 550)
    {
        if (comp == &lowBoostKnob)
            activeTooltipText = "LOW BOOST (0-10): Wide passive RC low shelf (+14.5 dB max). Pair with LOW ATTEN for the Pultec Low-End Trick.";
        else if (comp == &lowAttenKnob)
            activeTooltipText = "LOW ATTEN (0-10): Coupled 100k low cut (-17.5 dB max). When Low Boost is up, carves mud out of 180-450 Hz!";
        else if (comp == &lowFreqSelector)
            activeTooltipText = "LOW FREQUENCY (CPS): Authentic 4-tap capacitor switch (20, 30, 60, 100 CPS) serving both Low Boost & Low Atten.";
        else if (comp == &highBoostKnob)
            activeTooltipText = "HIGH BOOST (0-10): Silky toroidal RLC inductor bell boost (+15.5 dB max) with permalloy eddy-current smoothing.";
        else if (comp == &bandwidthKnob)
            activeTooltipText = "BANDWIDTH (0 Sharp .. 10 Broad): Damps the RLC inductor tank Q and peak height, exactly like the hardware.";
        else if (comp == &highBoostFreqSelector)
            activeTooltipText = "HIGH FREQUENCY (KCS): Authentic 7-tap inductor switch (3, 4, 5, 8, 10, 12, 16 KCS) for silky presence & air.";
        else if (comp == &highAttenKnob)
            activeTooltipText = "HIGH ATTEN (0-10): Independent passive RC high-shelf cut (-16 dB max) for velvety top-end control.";
        else if (comp == &highAttenFreqSelector)
            activeTooltipText = "ATTEN SEL (KCS): Independent high-shelf turnover selector (5, 10, 20 KCS).";
        else if (comp == &eqInSwitch)
            activeTooltipText = "IN / OUT SWITCH: Disables only the passive EQ filters while keeping all 3 transformers & 12AX7/12AU7 tubes active!";
        else if (comp == &powerSwitch)
            activeTooltipText = "OFF / ON (TRUE BYPASS): Switches the entire Pultec EQP-1A unit between active processing (ON) and true bypass (OFF).";
        else if (comp == &inputKnob)
            activeTooltipText = "INPUT (-18..+18 dB): SHIFT+LMB to auto-calibrate optimal input level (knob stays freely adjustable afterwards!).";
        else if (comp == &tubeDriveKnob)
            activeTooltipText = "TUBE & IRON DRIVE: Drives the Triad HS-56/HS-29, Peerless S-217-D transformers and 12AX7/12AU7 push-pull stage.";
        else if (comp == &outputKnob)
            activeTooltipText = "OUTPUT (-18..+18 dB): Clean master output trim.";
        else if (comp == &savePresetBtn)
            activeTooltipText = "SAVE PRESET: Save your current settings to a custom .pultec preset file.";
        else if (comp == &loadPresetBtn)
            activeTooltipText = "LOAD PRESET: Load any saved .pultec or .xml preset file from disk.";
        else
            activeTooltipText.clear();
    }
}

void PultecEQP1AAudioProcessorEditor::drawRadialDialNumbers(juce::Graphics& g,
                                                            const juce::Slider& slider,
                                                            const std::vector<juce::String>& labels,
                                                            float radiusOffset,
                                                            float fontSize)
{
    if (!slider.isVisible() || labels.empty()) return;

    const auto b = slider.getBounds().toFloat();
    const float cx = b.getCentreX();
    const float cy = b.getCentreY();
    const float baseR = std::min(b.getWidth(), b.getHeight()) * 0.5f + radiusOffset;

    float startA = 0.0f, endA = 0.0f;
    bool stopAtEnd = true;
    slider.getRotaryParameters(startA, endA, stopAtEnd);

    g.setColour(juce::Colour(0xfff4efe2));
    g.setFont(juce::Font(fontSize, juce::Font::bold));

    const int n = (int)labels.size();
    for (int i = 0; i < n; ++i)
    {
        const float t = (n > 1) ? (float)i / (float)(n - 1) : 0.5f;
        const float a = startA + t * (endA - startA) - juce::MathConstants<float>::halfPi;
        const float lx = cx + std::cos(a) * baseR;
        const float ly = cy + std::sin(a) * baseR;
        g.drawText(labels[(size_t)i], (int)(lx - 16.0f), (int)(ly - 7.0f), 32, 14, juce::Justification::centred, false);
    }
}

void PultecEQP1AAudioProcessorEditor::drawEQCurveDisplay(juce::Graphics& g, juce::Rectangle<float> area)
{
    juce::ColourGradient crtGrad(juce::Colour(0xff0a121a), area.getX(), area.getY(),
                                 juce::Colour(0xff111f2d), area.getX(), area.getBottom(), false);
    g.setGradientFill(crtGrad);
    g.fillRoundedRectangle(area, 6.0f);

    g.setColour(juce::Colour(0xff4c6b88));
    g.drawRoundedRectangle(area, 6.0f, 1.4f);

    const auto plot = area.reduced(36.0f, 16.0f);

    const double freqs[] = { 20.0, 30.0, 60.0, 100.0, 200.0, 500.0, 1000.0, 2000.0, 5000.0, 10000.0, 20000.0 };
    const char* labels[] = { "20", "30", "60", "100", "200", "500", "1k", "2k", "5k", "10k", "20k" };

    g.setFont(juce::Font(10.0f, juce::Font::plain));
    for (int i = 0; i < 11; ++i)
    {
        const float normX = (float)(std::log10(freqs[i] / 20.0) / std::log10(20000.0 / 20.0));
        const float px = plot.getX() + normX * plot.getWidth();
        g.setColour(juce::Colour(0xff23384d).withAlpha(0.7f));
        g.drawVerticalLine((int)px, plot.getY(), plot.getBottom());
        g.setColour(juce::Colour(0xff94b0cc));
        g.drawText(labels[i], (int)px - 15, (int)plot.getBottom() + 2, 30, 12, juce::Justification::centred);
    }

    for (int db = -18; db <= 18; db += 6)
    {
        const float normY = 0.5f - (float)db / 36.0f;
        const float py = plot.getY() + normY * plot.getHeight();
        g.setColour(db == 0 ? juce::Colour(0xff4c7090) : juce::Colour(0xff1b2d3e));
        g.drawHorizontalLine((int)py, plot.getX(), plot.getRight());
        g.setColour(juce::Colour(0xff94b0cc));
        g.drawText((db > 0 ? "+" : "") + juce::String(db), (int)area.getX() + 3, (int)py - 6, 29, 12, juce::Justification::centredRight);
    }

    juce::Path eqPath;
    juce::Path fillPath;
    const int numPts = 220;
    const float zeroY = plot.getCentreY();

    for (int i = 0; i <= numPts; ++i)
    {
        const float t = (float)i / (float)numPts;
        const double f = 20.0 * std::pow(1000.0, (double)t);
        const double magDb = std::clamp(audioProcessor.dspEngine.getMagnitudeDbAtFrequency(f), -18.0, 18.0);
        const float px = plot.getX() + t * plot.getWidth();
        const float py = plot.getY() + (0.5f - (float)magDb / 36.0f) * plot.getHeight();

        if (i == 0)
        {
            eqPath.startNewSubPath(px, py);
            fillPath.startNewSubPath(px, zeroY);
            fillPath.lineTo(px, py);
        }
        else
        {
            eqPath.lineTo(px, py);
            fillPath.lineTo(px, py);
        }
    }
    fillPath.lineTo(plot.getRight(), zeroY);
    fillPath.closeSubPath();

    juce::ColourGradient curveFill(juce::Colour(0xff44b4ff).withAlpha(0.30f), plot.getX(), plot.getY(),
                                   juce::Colour(0xffff9436).withAlpha(0.18f), plot.getX(), plot.getBottom(), false);
    g.setGradientFill(curveFill);
    g.fillPath(fillPath);

    g.setColour(juce::Colour(0xff68c8ff));
    g.strokePath(eqPath, juce::PathStrokeType(2.4f));

    // THD Harmonic Saturation Meter on right edge of analyzer
    const float harm = std::clamp(audioProcessor.dspEngine.getHarmonicMeter(), 0.0f, 1.0f);
    juce::Rectangle<float> thdBox(area.getRight() - 28.0f, plot.getY(), 18.0f, plot.getHeight());
    g.setColour(juce::Colour(0xff0d151d));
    g.fillRoundedRectangle(thdBox, 3.0f);
    const float fillH = thdBox.getHeight() * harm;
    juce::ColourGradient thdGrad(juce::Colour(0xffff481c), thdBox.getX(), thdBox.getY(),
                                 juce::Colour(0xffffbe3b), thdBox.getX(), thdBox.getBottom(), false);
    g.setGradientFill(thdGrad);
    g.fillRoundedRectangle(thdBox.getX(), thdBox.getBottom() - fillH, thdBox.getWidth(), fillH, 3.0f);
    g.setColour(juce::Colour(0xff94b0cc));
    g.setFont(juce::Font(9.5f, juce::Font::bold));
    g.drawText("THD", (int)thdBox.getX() - 4, (int)plot.getBottom() + 2, 26, 12, juce::Justification::centred);
}

void PultecEQP1AAudioProcessorEditor::drawRackChassis(juce::Graphics& g, juce::Rectangle<float> area)
{
    // Authentic Pultec EQP-1A Vintage Teal-Blue Faceplate (color-matched to image_f1567c.png!)
    juce::ColourGradient panelGrad(juce::Colour(0xff38647c), area.getX(), area.getY(),
                                   juce::Colour(0xff2b5166), area.getX(), area.getBottom(), false);
    panelGrad.addColour(0.5, juce::Colour(0xff325a70));
    g.setGradientFill(panelGrad);
    g.fillRoundedRectangle(area, 4.0f);

    // Subtle metallic border bevel
    g.setColour(juce::Colour(0xff6b94ab).withAlpha(0.55f));
    g.drawRoundedRectangle(area.reduced(1.0f), 4.0f, 1.5f);

    // 4 Chrome Rack-Mount Slotted Screws at the left and right edges (just like image_f1567c.png)
    auto drawChromeScrew = [&](float sx, float sy)
    {
        g.setColour(juce::Colours::black.withAlpha(0.55f));
        g.fillEllipse(sx - 8.5f, sy - 7.5f, 17.0f, 17.0f);
        juce::ColourGradient scGrad(juce::Colour(0xfff2f5f8), sx - 8.0f, sy - 8.0f,
                                    juce::Colour(0xff7d8790), sx + 8.0f, sy + 8.0f, false);
        g.setGradientFill(scGrad);
        g.fillEllipse(sx - 8.0f, sy - 8.0f, 16.0f, 16.0f);
        g.setColour(juce::Colour(0xff363d44));
        g.drawEllipse(sx - 8.0f, sy - 8.0f, 16.0f, 16.0f, 1.0f);
        g.drawLine(sx - 5.5f, sy - 2.0f, sx + 5.5f, sy + 2.0f, 1.8f);
    };

    drawChromeScrew(area.getX() + 18.0f, area.getY() + 72.0f);
    drawChromeScrew(area.getX() + 18.0f, area.getBottom() - 48.0f);
    drawChromeScrew(area.getRight() - 18.0f, area.getY() + 72.0f);
    drawChromeScrew(area.getRight() - 18.0f, area.getBottom() - 48.0f);

    const float ry = area.getY();

    // Engraved White/Cream Silkscreen Titles matching image_f1567c.png
    g.setColour(juce::Colour(0xfff4efe2));
    g.setFont(juce::Font(13.0f, juce::Font::bold));

    // Top row labels: BOOST | ATTEN | BOOST | ATTEN | ATTEN SEL
    g.drawText("BOOST",     (int)lowBoostKnob.getX() - 10,          (int)ry + 18, 128, 18, juce::Justification::centred);
    g.drawText("ATTEN",     (int)lowAttenKnob.getX() - 10,          (int)ry + 18, 128, 18, juce::Justification::centred);
    g.drawText("BOOST",     (int)highBoostKnob.getX() - 10,         (int)ry + 18, 128, 18, juce::Justification::centred);
    g.drawText("ATTEN",     (int)highAttenKnob.getX() - 10,         (int)ry + 18, 128, 18, juce::Justification::centred);
    g.drawText("ATTEN SEL", (int)highAttenFreqSelector.getX() - 16, (int)ry + 18, 128, 18, juce::Justification::centred);

    // Bottom row labels: IN | CPS + LOW FREQUENCY | BANDWIDTH (SHARP / BROAD) | KCS + HIGH FREQUENCY | OFF / ON
    g.drawText("IN",             (int)eqInSwitch.getX() - 6,             (int)eqInSwitch.getY() - 22, 60, 16, juce::Justification::centred);
    g.drawText("CPS",            (int)lowFreqSelector.getX() - 10,       (int)lowFreqSelector.getY() - 36, 116, 16, juce::Justification::centred);
    g.drawText("LOW FREQUENCY",  (int)lowFreqSelector.getX() - 28,       (int)lowFreqSelector.getBottom() + 8, 152, 16, juce::Justification::centred);

    g.drawText("BANDWIDTH",      (int)bandwidthKnob.getX() - 20,         (int)bandwidthKnob.getBottom() + 8, 148, 16, juce::Justification::centred);
    g.setFont(juce::Font(9.5f, juce::Font::bold));
    g.drawText("SHARP",          (int)bandwidthKnob.getX() - 22,         (int)bandwidthKnob.getBottom() - 4, 48, 12, juce::Justification::centred);
    g.drawText("BROAD",          (int)bandwidthKnob.getRight() - 26,     (int)bandwidthKnob.getBottom() - 4, 48, 12, juce::Justification::centred);

    g.setFont(juce::Font(13.0f, juce::Font::bold));
    g.drawText("KCS",            (int)highBoostFreqSelector.getX() - 10, (int)highBoostFreqSelector.getY() - 38, 116, 16, juce::Justification::centred);
    g.drawText("HIGH FREQUENCY", (int)highBoostFreqSelector.getX() - 28, (int)highBoostFreqSelector.getBottom() + 8, 152, 16, juce::Justification::centred);

    g.setFont(juce::Font(11.5f, juce::Font::bold));
    g.drawText("OFF",            (int)powerSwitch.getX() - 12,           (int)powerSwitch.getY() - 16, 40, 14, juce::Justification::centred);
    g.drawText("ON",             (int)powerSwitch.getRight() - 24,       (int)powerSwitch.getY() - 10, 40, 14, juce::Justification::centred);

    // Subtle vintage Pulse Techniques badge on top-center of rack
    g.setColour(juce::Colour(0xfff4efe2).withAlpha(0.75f));
    g.setFont(juce::Font(11.0f, juce::Font::bold));
    g.drawText("PULTEC PROGRAM EQUALIZER EQP-1A", 340, (int)ry + 4, 280, 14, juce::Justification::centred);

    // Radial number markings around all 8 rotary controls (exactly like image_f1567c.png)
    const std::vector<juce::String> scale0to10 = { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10" };
    drawRadialDialNumbers(g, lowBoostKnob,  scale0to10, 4.0f, 10.5f);
    drawRadialDialNumbers(g, lowAttenKnob,  scale0to10, 4.0f, 10.5f);
    drawRadialDialNumbers(g, highBoostKnob, scale0to10, 4.0f, 10.5f);
    drawRadialDialNumbers(g, highAttenKnob, scale0to10, 4.0f, 10.5f);
    drawRadialDialNumbers(g, bandwidthKnob, scale0to10, 4.0f, 10.5f);

    drawRadialDialNumbers(g, lowFreqSelector,       { "20", "30", "60", "100" },                  6.0f, 11.0f);
    drawRadialDialNumbers(g, highBoostFreqSelector, { "3", "4", "5", "8", "10", "12", "16" },     7.0f, 11.0f);
    drawRadialDialNumbers(g, highAttenFreqSelector, { "5", "10", "20" },                          6.0f, 11.0f);

    // Red Faceted Ruby Jewel Pilot Lamp (positioned above OFF/ON switch, below ATTEN SEL, matching image_f1567c.png)
    const bool active = !powerSwitch.getToggleState(); // bypass == false means ON
    const float harmGlow = std::clamp(audioProcessor.dspEngine.getHarmonicMeter(), 0.0f, 1.0f);
    const juce::Rectangle<float> jewel(756.0f, ry + 186.0f, 38.0f, 38.0f);

    if (active)
    {
        juce::ColourGradient glow(juce::Colour(0xffff2e17).withAlpha(0.70f + 0.30f * harmGlow),
                                  jewel.getCentreX(), jewel.getCentreY(),
                                  juce::Colours::transparentBlack,
                                  jewel.getCentreX() + 34.0f, jewel.getCentreY() + 34.0f, true);
        g.setGradientFill(glow);
        g.fillEllipse(jewel.expanded(15.0f));
    }

    // Chrome bezel ring around ruby lamp
    juce::ColourGradient ringGrad(juce::Colour(0xfff0f4f8), jewel.getX(), jewel.getY(),
                                  juce::Colour(0xff6c7680), jewel.getRight(), jewel.getBottom(), false);
    g.setGradientFill(ringGrad);
    g.fillEllipse(jewel);
    g.setColour(juce::Colour(0xff232a30));
    g.drawEllipse(jewel, 1.3f);

    // Ruby faceted glass dome
    const auto innerRuby = jewel.reduced(5.5f);
    juce::ColourGradient rubyGrad(active ? juce::Colour(0xffff4830).brighter(0.35f * harmGlow) : juce::Colour(0xff4a1512),
                                  innerRuby.getX() + 6.0f, innerRuby.getY() + 6.0f,
                                  active ? juce::Colour(0xff8f0c06) : juce::Colour(0xff1e0706),
                                  innerRuby.getRight(), innerRuby.getBottom(), false);
    g.setGradientFill(rubyGrad);
    g.fillEllipse(innerRuby);

    // Facet specular highlight
    g.setColour(juce::Colours::white.withAlpha(active ? 0.65f : 0.18f));
    g.fillEllipse(innerRuby.getX() + 5.0f, innerRuby.getY() + 4.0f, 8.0f, 6.0f);
}

void PultecEQP1AAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff101820));

    // 1. If Expanded: Draw the Slide-Out Top Mastering & Analyzer HUD
    if (isHudExpanded)
    {
        juce::Rectangle<float> topArea(10.0f, 8.0f, (float)getWidth() - 20.0f, (float)kHudHeight - 12.0f);
        drawEQCurveDisplay(g, topArea.withTrimmedRight(390.0f));

        juce::Rectangle<float> ctrlBox(topArea.getRight() - 380.0f, topArea.getY(), 380.0f, topArea.getHeight());
        g.setColour(juce::Colour(0xff15222e));
        g.fillRoundedRectangle(ctrlBox, 6.0f);
        g.setColour(juce::Colour(0xff4c6b88));
        g.drawRoundedRectangle(ctrlBox, 6.0f, 1.4f);

        g.setColour(juce::Colour(0xfff4efe2));
        g.setFont(juce::Font(13.5f, juce::Font::bold));
        g.drawText("MASTERING HUD & PRESET MANAGER", (int)ctrlBox.getX() + 10, (int)ctrlBox.getY() + 6, 360, 18, juce::Justification::centred);

        // Labels for Trim Knobs inside HUD
        auto drawTrimLabel = [&](juce::Slider& s, const juce::String& name)
        {
            auto b = s.getBounds();
            g.setColour(juce::Colour(0xffb8d0e8));
            g.setFont(juce::Font(10.0f, juce::Font::bold));
            g.drawText(name, b.getX() - 14, b.getY() - 13, b.getWidth() + 28, 12, juce::Justification::centred);
            g.setColour(juce::Colour(0xfff4efe2));
            g.setFont(juce::Font(10.5f, juce::Font::plain));
            g.drawText(juce::String(s.getValue(), 1) + " dB", b.getX() - 10, b.getBottom() - 2, b.getWidth() + 20, 13, juce::Justification::centred);
        };

        drawTrimLabel(inputKnob,     "INPUT (Shift+LMB)");
        drawTrimLabel(tubeDriveKnob, "TUBE DRIVE");
        drawTrimLabel(outputKnob,    "OUTPUT");
    }

    // 2. Draw Authentic Pultec EQP-1A Hardware Rack Faceplate
    const float rackY = isHudExpanded ? (float)kHudHeight : 0.0f;
    juce::Rectangle<float> rackArea(4.0f, rackY + 4.0f, (float)getWidth() - 8.0f, (float)kRackHeight - 8.0f);
    drawRackChassis(g, rackArea);

    // 3. Shift+LMB Input Auto-Calibration Overlay Modal
    if (showCalibrationBanner || audioProcessor.dspEngine.isCalibrationActive())
    {
        auto modal = getLocalBounds().toFloat().withSizeKeepingCentre(470.0f, 118.0f);
        g.setColour(juce::Colour(0xf00d151e));
        g.fillRoundedRectangle(modal, 8.0f);
        g.setColour(juce::Colour(0xff5ec2ff));
        g.drawRoundedRectangle(modal, 8.0f, 2.0f);

        g.setColour(juce::Colour(0xfff5efe0));
        g.setFont(juce::Font(15.0f, juce::Font::bold));
        g.drawText("INPUT AUTO-CALIBRATION (TRIAD HS-56 SWEET SPOT)", modal.removeFromTop(34.0f), juce::Justification::centred);

        g.setFont(juce::Font(13.0f, juce::Font::plain));
        if (audioProcessor.dspEngine.isCalibrationWaiting())
        {
            g.setColour(juce::Colour(0xffffcc55));
            g.drawText("Play your audio in DAW so we can calibrate optimal Input drive...", modal.reduced(16.0f, 8.0f), juce::Justification::centred);
        }
        else if (audioProcessor.dspEngine.isCalibrationActive())
        {
            const float prog = audioProcessor.dspEngine.getCalibrationProgress();
            g.setColour(juce::Colour(0xff9cd6ff));
            g.drawText("Listening & calibrating transformer headroom... " + juce::String((int)(prog * 100.0f)) + "%",
                       modal.removeFromTop(32.0f), juce::Justification::centred);
            auto bar = modal.withSizeKeepingCentre(360.0f, 14.0f);
            g.setColour(juce::Colour(0xff1e2f40));
            g.fillRoundedRectangle(bar, 4.0f);
            g.setColour(juce::Colour(0xff42b0ff));
            g.fillRoundedRectangle(bar.withWidth(bar.getWidth() * prog), 4.0f);
        }
        else
        {
            g.setColour(juce::Colour(0xff62e884));
            g.drawText("CALIBRATION COMPLETE! Optimal Input set to "
                       + juce::String(audioProcessor.dspEngine.getRecommendedInputDb(), 1) + " dB (Knob Unlocked)",
                       modal, juce::Justification::centred);
        }
    }

    // 4. Hover Tooltip Bar
    if (activeTooltipText.isNotEmpty())
    {
        auto tipRect = juce::Rectangle<float>(44.0f, (float)getHeight() - 26.0f, (float)getWidth() - 88.0f, 22.0f);
        g.setColour(juce::Colour(0xee0d1620));
        g.fillRoundedRectangle(tipRect, 4.0f);
        g.setColour(juce::Colour(0xff68c8ff).withAlpha(0.7f));
        g.drawRoundedRectangle(tipRect, 4.0f, 1.0f);
        g.setColour(juce::Colour(0xfff4efe2));
        g.setFont(juce::Font(11.5f, juce::Font::plain));
        g.drawText(activeTooltipText, tipRect.reduced(8.0f, 0.0f), juce::Justification::centred);
    }
}

void PultecEQP1AAudioProcessorEditor::resized()
{
    const int ry = isHudExpanded ? kHudHeight : 0;

    // Expand / Collapse HUD button sits neatly in the top-right bezel of the rack faceplate
    expandHudButton.setBounds(kWidth - 265, ry + 7, 232, 22);

    // Show/hide top HUD controls based on expansion state
    for (juce::Component* c : { (juce::Component*)&presetBox, &savePresetBtn, &loadPresetBtn,
                                &routingBox, &oversamplingBox, &inputKnob, &tubeDriveKnob,
                                &outputKnob, &autoGainButton, &analogHumButton })
    {
        c->setVisible(isHudExpanded);
    }

    if (isHudExpanded)
    {
        const int hx = kWidth - 386;
        presetBox.setBounds(hx + 12, 30, 236, 24);
        savePresetBtn.setBounds(hx + 254, 30, 54, 24);
        loadPresetBtn.setBounds(hx + 312, 30, 54, 24);

        routingBox.setBounds(hx + 12, 60, 174, 24);
        oversamplingBox.setBounds(hx + 192, 60, 174, 24);

        inputKnob.setBounds(hx + 22,  104, 64, 64);
        tubeDriveKnob.setBounds(hx + 114, 104, 64, 64);
        outputKnob.setBounds(hx + 204, 104, 64, 64);

        autoGainButton.setBounds(hx + 282, 106, 86, 28);
        analogHumButton.setBounds(hx + 282, 142, 86, 28);
    }

    //==========================================================================
    // AUTHENTIC PULTEC EQP-1A FRONT-PANEL GEOMETRY (Exact match to image_f1567c.png)
    //==========================================================================
    // TOP ROW: [LOW BOOST]   [LOW ATTEN]      [HIGH BOOST]   [HIGH ATTEN]      [ATTEN SEL]
    const int topRowY = ry + 48;
    lowBoostKnob.setBounds         ( 76, topRowY, 108, 108);
    lowAttenKnob.setBounds         (236, topRowY, 108, 108);
    highBoostKnob.setBounds        (436, topRowY, 108, 108);
    highAttenKnob.setBounds        (596, topRowY, 108, 108);
    highAttenFreqSelector.setBounds(776, topRowY + 6, 96, 96);

    // BOTTOM ROW: [IN Toggle]   [LOW FREQ CPS]   [BANDWIDTH]   [HIGH FREQ KCS]   [OFF/ON Switch]
    const int botRowY = ry + 206;
    eqInSwitch.setBounds           ( 36, botRowY + 34, 48, 54);
    lowFreqSelector.setBounds      (162, botRowY + 6,  96, 96);
    bandwidthKnob.setBounds        (336, botRowY - 2, 108, 108);
    highBoostFreqSelector.setBounds(522, botRowY + 6,  96, 96);
    powerSwitch.setBounds          (806, botRowY + 36, 56, 56);
}
