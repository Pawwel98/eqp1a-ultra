#include "PluginProcessor.h"
#include "PluginEditor.h"

namespace
{
    // Deterministic hash noise for procedural rust, scratches, and metal wear
    inline float hash2D(int x, int y) noexcept
    {
        unsigned int n = (unsigned int)(x * 374761393 + y * 668265263);
        n = (n ^ (n >> 13)) * 1274126177u;
        return (float)(n & 0x00ffffffu) / 16777215.0f;
    }

    inline float valueNoise(float x, float y) noexcept
    {
        const int ix = (int)std::floor(x);
        const int iy = (int)std::floor(y);
        const float fx = x - (float)ix;
        const float fy = y - (float)iy;
        const float u = fx * fx * (3.0f - 2.0f * fx);
        const float v = fy * fy * (3.0f - 2.0f * fy);
        const float a = hash2D(ix, iy);
        const float b = hash2D(ix + 1, iy);
        const float c = hash2D(ix, iy + 1);
        const float d = hash2D(ix + 1, iy + 1);
        return a + u * (b - a) + v * (c - a) + u * v * (a - b - c + d);
    }

    inline float fbmNoise(float x, float y) noexcept
    {
        float val = 0.0f;
        float amp = 0.5f;
        for (int i = 0; i < 4; ++i)
        {
            val += amp * valueNoise(x, y);
            x *= 2.05f;
            y *= 2.05f;
            amp *= 0.5f;
        }
        return val;
    }
}

//==============================================================================
PultecRackLookAndFeel::PultecRackLookAndFeel()
{
    setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff14110f));
    setColour(juce::ComboBox::textColourId,       juce::Colour(0xffe8dcc8));
    setColour(juce::ComboBox::outlineColourId,    juce::Colour(0xff6e482b));
    setColour(juce::PopupMenu::backgroundColourId,juce::Colour(0xff161310));
    setColour(juce::PopupMenu::textColourId,      juce::Colour(0xffe8dcc8));
    setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xff7a3b18));
}

void PultecRackLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool,
                                         int, int, int, int, juce::ComboBox& box)
{
    auto r = juce::Rectangle<float>(0.0f, 0.0f, (float)width, (float)height).reduced(1.0f);

    // Industrial dark gunmetal & rusted-edge cage matching the amplifier grille
    juce::ColourGradient bgGrad(juce::Colour(0xff221d1a), r.getX(), r.getY(),
                                juce::Colour(0xff100d0b), r.getX(), r.getBottom(), false);
    g.setGradientFill(bgGrad);
    g.fillRoundedRectangle(r, 3.0f);

    // Rusted bronze/iron bezel outline
    g.setColour(juce::Colour(0xff784625));
    g.drawRoundedRectangle(r, 3.0f, 1.3f);
    g.setColour(juce::Colour(0xffa86434).withAlpha(0.45f));
    g.drawHorizontalLine(2, r.getX() + 3.0f, r.getRight() - 3.0f);

    // Industrial screw rivets on left and right of preset/mode bar
    g.setColour(juce::Colour(0xff4e3624));
    g.fillEllipse(r.getX() + 4.0f, r.getCentreY() - 2.5f, 5.0f, 5.0f);

    // Amber tube-style indicator arrow on right
    const float ax = r.getRight() - 14.0f;
    const float ay = r.getCentreY();
    juce::Path arrow;
    arrow.addTriangle(ax - 4.0f, ay - 2.5f, ax + 4.0f, ay - 2.5f, ax, ay + 3.5f);
    g.setColour(juce::Colour(0xffff9638));
    g.fillPath(arrow);
    g.setFont(juce::Font(11.5f, juce::Font::bold));
    (void)box;
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
    const int seed  = slider.getX() * 17 + slider.getY() * 31;

    if (style == StyleFlutedSkirted || style == StyleTrimKnob)
    {
        // Deep ambient drop shadow on rusted faceplate
        g.setColour(juce::Colours::black.withAlpha(0.68f));
        g.fillEllipse(cx - radius * 0.78f + 3.5f, cy - radius * 0.78f + 5.5f, radius * 1.56f, radius * 1.56f);

        // 1. Time-Worn Tarnished Silver Skirt with Rust & Patina Spots
        const float skirtR = radius * 0.77f;
        juce::ColourGradient skirtGrad(juce::Colour(0xffb8b0a2), cx - skirtR * 0.8f, cy - skirtR * 0.8f,
                                       juce::Colour(0xff3e3932), cx + skirtR * 0.8f, cy + skirtR * 0.8f, false);
        skirtGrad.addColour(0.45, juce::Colour(0xff847c6e));
        skirtGrad.addColour(0.75, juce::Colour(0xff5a4838)); // Oxidized warm brass/rust tint
        g.setGradientFill(skirtGrad);
        g.fillEllipse(cx - skirtR, cy - skirtR, skirtR * 2.0f, skirtR * 2.0f);

        // Weathering & rust spots rotating with the metallic skirt
        for (int sp = 0; sp < 14; ++sp)
        {
            const float h1 = hash2D(seed + sp * 7, sp * 13);
            const float spotA = angle + h1 * juce::MathConstants<float>::twoPi;
            const float spotDist = skirtR * (0.64f + 0.30f * hash2D(sp, seed));
            const float sx = cx + std::cos(spotA) * spotDist;
            const float sy = cy + std::sin(spotA) * spotDist;
            const float sr = 2.0f + 3.5f * hash2D(sp * 3, seed + 5);
            g.setColour((sp % 2 == 0 ? juce::Colour(0xff5e2c14) : juce::Colour(0xff221c18)).withAlpha(0.42f));
            g.fillEllipse(sx - sr * 0.5f, sy - sr * 0.5f, sr, sr * 0.75f);
        }

        // Scratched metallic rim highlight & dark oxidized bevel
        g.setColour(juce::Colour(0xff1e1a17));
        g.drawEllipse(cx - skirtR, cy - skirtR, skirtR * 2.0f, skirtR * 2.0f, 1.6f);
        g.setColour(juce::Colour(0xffdfd8c8).withAlpha(0.45f));
        g.drawEllipse(cx - skirtR + 1.2f, cy - skirtR + 1.2f, (skirtR - 1.2f) * 2.0f, (skirtR - 1.2f) * 2.0f, 0.9f);

        // 2. Worn Black Bakelite Fluted Body
        const float capR = radius * 0.61f;
        juce::ColourGradient capGrad(juce::Colour(0xff383634), cx - capR * 0.6f, cy - capR * 0.6f,
                                     juce::Colour(0xff100f0e), cx + capR * 0.7f, cy + capR * 0.7f, false);
        g.setGradientFill(capGrad);
        g.fillEllipse(cx - capR, cy - capR, capR * 2.0f, capR * 2.0f);

        // 10 Scalloped Grip Flutes with worn silver/steel edge scuffing
        for (int r = 0; r < 10; ++r)
        {
            const float ribA = angle + (float)r * (juce::MathConstants<float>::twoPi / 10.0f) - juce::MathConstants<float>::halfPi;
            const float fx = cx + std::cos(ribA) * (capR * 0.95f);
            const float fy = cy + std::sin(ribA) * (capR * 0.95f);
            g.setColour(juce::Colour(0xff090909).withAlpha(0.90f));
            g.fillEllipse(fx - 3.8f, fy - 3.8f, 7.6f, 7.6f);
            // Worn metallic edge highlight on each flute
            g.setColour(juce::Colour(0xff9e9688).withAlpha(0.42f));
            g.drawEllipse(fx - 3.8f, fy - 3.8f, 7.6f, 7.6f, 0.9f);
        }

        // Inner concave bakelite cap with battle scratches
        const float innerR = capR * 0.58f;
        juce::ColourGradient innerGrad(juce::Colour(0xff161514), cx - innerR, cy - innerR,
                                       juce::Colour(0xff2e2b28), cx + innerR, cy + innerR, false);
        g.setGradientFill(innerGrad);
        g.fillEllipse(cx - innerR, cy - innerR, innerR * 2.0f, innerR * 2.0f);

        // Worn metal ring around center cap
        g.setColour(juce::Colour(0xff8c8274).withAlpha(0.55f));
        g.drawEllipse(cx - innerR, cy - innerR, innerR * 2.0f, innerR * 2.0f, 1.2f);

        // Micro-scratches across the black center cap
        g.setColour(juce::Colour(0xffb8b0a4).withAlpha(0.32f));
        for (int sc = 0; sc < 5; ++sc)
        {
            const float sa = angle + hash2D(seed, sc * 19) * 3.14f;
            const float len = innerR * (0.35f + 0.35f * hash2D(sc, seed));
            g.drawLine(cx - std::cos(sa) * len, cy - std::sin(sa) * len,
                       cx + std::cos(sa) * len, cy + std::sin(sa) * len, 0.8f);
        }

        // Aged cream/ivory engraved indicator line
        const float indA = angle - juce::MathConstants<float>::halfPi;
        g.setColour(juce::Colour(0xff14110e));
        g.drawLine(cx + std::cos(indA) * (innerR * 0.92f) + 1.0f, cy + std::sin(indA) * (innerR * 0.92f) + 1.0f,
                   cx + std::cos(indA) * (skirtR * 0.97f) + 1.0f, cy + std::sin(indA) * (skirtR * 0.97f) + 1.0f, 3.0f);
        g.setColour(juce::Colour(0xffe8dec8));
        g.drawLine(cx + std::cos(indA) * (innerR * 0.92f), cy + std::sin(indA) * (innerR * 0.92f),
                   cx + std::cos(indA) * (skirtR * 0.97f), cy + std::sin(indA) * (skirtR * 0.97f), 2.3f);
    }
    else // StyleChickenHead (Weathered vintage bakelite pointer switch)
    {
        const float baseR = radius * 0.45f;
        g.setColour(juce::Colours::black.withAlpha(0.62f));
        g.fillEllipse(cx - baseR + 3.0f, cy - baseR + 5.0f, baseR * 2.0f, baseR * 2.0f);

        // Oxidized brass/steel hex bushing under switch
        g.setColour(juce::Colour(0xff786856));
        g.fillEllipse(cx - baseR * 0.76f, cy - baseR * 0.76f, baseR * 1.52f, baseR * 1.52f);
        g.setColour(juce::Colour(0xff38281c));
        g.drawEllipse(cx - baseR * 0.76f, cy - baseR * 0.76f, baseR * 1.52f, baseR * 1.52f, 1.2f);

        juce::Path pointer;
        pointer.startNewSubPath(-baseR * 0.40f,  baseR * 0.68f);
        pointer.lineTo         ( baseR * 0.40f,  baseR * 0.68f);
        pointer.lineTo         ( baseR * 0.22f, -radius * 0.76f);
        pointer.lineTo         (-baseR * 0.22f, -radius * 0.76f);
        pointer.closeSubPath();

        g.setColour(juce::Colours::black.withAlpha(0.55f));
        g.fillPath(pointer, juce::AffineTransform::rotation(angle).translated(cx + 3.5f, cy + 5.5f));

        juce::ColourGradient ptrGrad(juce::Colour(0xff383532), cx - baseR, cy - baseR,
                                     juce::Colour(0xff0e0d0c), cx + baseR, cy + baseR, false);
        g.setGradientFill(ptrGrad);
        g.fillPath(pointer, juce::AffineTransform::rotation(angle).translated(cx, cy));

        // Scuffed metal edges on the bakelite pointer
        g.setColour(juce::Colour(0xff8e8476).withAlpha(0.65f));
        g.strokePath(pointer, juce::PathStrokeType(1.3f), juce::AffineTransform::rotation(angle).translated(cx, cy));

        g.setColour(juce::Colour(0xff1a1816));
        g.fillEllipse(cx - baseR * 0.54f, cy - baseR * 0.54f, baseR * 1.08f, baseR * 1.08f);
        g.setColour(juce::Colour(0xff6e655a));
        g.drawEllipse(cx - baseR * 0.54f, cy - baseR * 0.54f, baseR * 1.08f, baseR * 1.08f, 1.0f);

        const float indA = angle - juce::MathConstants<float>::halfPi;
        g.setColour(juce::Colour(0xffe8dec8));
        g.drawLine(cx + std::cos(indA) * (baseR * 0.28f), cy + std::sin(indA) * (baseR * 0.28f),
                   cx + std::cos(indA) * (radius * 0.73f), cy + std::sin(indA) * (radius * 0.73f), 2.2f);
    }
}

void PultecRackLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                                             bool isHighlighted, bool)
{
    const auto name = button.getButtonText();
    auto r = button.getLocalBounds().toFloat();

    // Both IN/OUT (left) and OFF/ON (right) are weathered chrome bat toggles matching the design!
    if (name == juce::String("IN") || name == juce::String("ON"))
    {
        const float cx = r.getCentreX();
        const float cy = r.getCentreY() + 2.0f;
        const bool isUp = (name == juce::String("IN")) ? button.getToggleState() : !button.getToggleState();

        g.setColour(juce::Colours::black.withAlpha(0.6f));
        g.fillEllipse(cx - 15.0f, cy - 13.0f, 30.0f, 30.0f);

        // Tarnished brass/steel hex nut & washer
        juce::ColourGradient nutGrad(juce::Colour(0xffc8bfb0), cx - 13.0f, cy - 13.0f,
                                     juce::Colour(0xff4e443a), cx + 13.0f, cy + 13.0f, false);
        nutGrad.addColour(0.6, juce::Colour(0xff78624c));
        g.setGradientFill(nutGrad);
        g.fillEllipse(cx - 13.5f, cy - 13.5f, 27.0f, 27.0f);
        g.setColour(juce::Colour(0xff241c16));
        g.drawEllipse(cx - 13.5f, cy - 13.5f, 27.0f, 27.0f, 1.4f);

        g.setColour(juce::Colour(0xff141210));
        g.fillEllipse(cx - 7.0f, cy - 7.0f, 14.0f, 14.0f);

        const float tipY = isUp ? (cy - 17.0f) : (cy + 17.0f);
        g.setColour(juce::Colours::black.withAlpha(0.50f));
        g.drawLine(cx + 2.5f, cy + 2.0f, cx + 2.5f, tipY + 3.0f, 7.5f);

        g.setColour(juce::Colour(0xffc4bcb0));
        g.drawLine(cx, cy, cx, tipY, 6.2f);
        g.setColour(juce::Colour(0xffebe4d8));
        g.fillEllipse(cx - 4.5f, tipY - 4.5f, 9.0f, 9.0f);
        g.setColour(juce::Colour(0xff3a322b));
        g.drawEllipse(cx - 4.5f, tipY - 4.5f, 9.0f, 9.0f, 1.0f);
        return;
    }

    // Industrial Tube-Bay Toggle Button (AUTO GAIN / 60Hz HUM)
    auto box = r.reduced(2.0f);
    const bool on = button.getToggleState();
    juce::Colour bg = on ? juce::Colour(0xff5e2c14) : juce::Colour(0xff1a1614);
    if (isHighlighted) bg = bg.brighter(0.12f);

    g.setColour(bg);
    g.fillRoundedRectangle(box, 3.0f);
    g.setColour(on ? juce::Colour(0xffff9638) : juce::Colour(0xff6e482b));
    g.drawRoundedRectangle(box, 3.0f, 1.2f);

    // Glowing amber tube-filament LED dot
    g.setColour(on ? juce::Colour(0xffffaa33) : juce::Colour(0xff382c24));
    g.fillEllipse(box.getX() + 7.0f, box.getCentreY() - 3.5f, 7.0f, 7.0f);

    g.setColour(juce::Colour(0xffe8dcc8));
    g.setFont(juce::Font(10.5f, juce::Font::bold));
    g.drawText(name, box.withTrimmedLeft(16.0f), juce::Justification::centred, false);
}

void PultecRackLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                                 const juce::Colour&, bool isHighlighted, bool isDown)
{
    auto r = button.getLocalBounds().toFloat().reduced(1.5f);
    juce::Colour bg = juce::Colour(0xff221c18);
    if (button.getButtonText().contains("TUBE BAY"))
        bg = juce::Colour(0xff281f1a);
    if (isHighlighted) bg = bg.brighter(0.14f);
    if (isDown)        bg = bg.darker(0.15f);

    g.setColour(bg);
    g.fillRoundedRectangle(r, 3.5f);

    // Rusted steel border + corner rivets
    g.setColour(juce::Colour(0xff8c522b));
    g.drawRoundedRectangle(r, 3.5f, 1.3f);
    g.setColour(juce::Colour(0xffff9c3b).withAlpha(0.35f));
    g.drawHorizontalLine((int)r.getY() + 1, r.getX() + 3.0f, r.getRight() - 3.0f);
}

//==============================================================================
PultecEQP1AAudioProcessorEditor::PultecEQP1AAudioProcessorEditor(PultecEQP1AAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setLookAndFeel(&pultecLNF);
    generateProceduralTextures();

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

    configDial(lowBoostKnob,  PultecRackLookAndFeel::StyleFlutedSkirted, 0.0, stdStart, stdEnd);
    configDial(lowAttenKnob,  PultecRackLookAndFeel::StyleFlutedSkirted, 0.0, stdStart, stdEnd);
    configDial(highBoostKnob, PultecRackLookAndFeel::StyleFlutedSkirted, 0.0, stdStart, stdEnd);
    configDial(highAttenKnob, PultecRackLookAndFeel::StyleFlutedSkirted, 0.0, stdStart, stdEnd);
    configDial(bandwidthKnob, PultecRackLookAndFeel::StyleFlutedSkirted, 5.0, stdStart, stdEnd);

    configDial(lowFreqSelector, PultecRackLookAndFeel::StyleChickenHead, 1.0,
               juce::MathConstants<float>::twoPi - 0.90f,
               juce::MathConstants<float>::twoPi + 0.90f);

    configDial(highBoostFreqSelector, PultecRackLookAndFeel::StyleChickenHead, 5.0,
               juce::MathConstants<float>::twoPi - 1.52f,
               juce::MathConstants<float>::twoPi + 1.52f);

    configDial(highAttenFreqSelector, PultecRackLookAndFeel::StyleChickenHead, 2.0,
               juce::MathConstants<float>::twoPi - 0.62f,
               juce::MathConstants<float>::twoPi + 0.62f);

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

    expandHudButton.setButtonText("[+] EXPAND TUBE BAY");
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

    isHudExpanded = false;
    setSize(kWidth, kRackHeight);

    startTimerHz(30);
}

PultecEQP1AAudioProcessorEditor::~PultecEQP1AAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void PultecEQP1AAudioProcessorEditor::generateProceduralTextures()
{
    // 1. Pre-render Weathered Aquamarine-Blue Steel Chassis with Organic Rust & Scratches
    weatheredRackImage = juce::Image(juce::Image::ARGB, kWidth, kRackHeight, true);
    {
        juce::Graphics g(weatheredRackImage);

        for (int y = 0; y < kRackHeight; ++y)
        {
            for (int x = 0; x < kWidth; ++x)
            {
                const float nx = (float)x / (float)kWidth;
                const float ny = (float)y / (float)kRackHeight;

                // Distance to chassis edges & corner screws for authentic rust accumulation
                const float edgeDistX = std::min(nx, 1.0f - nx);
                const float edgeDistY = std::min(ny, 1.0f - ny);
                const float borderProximity = std::clamp(1.0f - std::min(edgeDistX * 14.0f, edgeDistY * 8.5f), 0.0f, 1.0f);

                // Screw proximity rust (4 corner screws)
                float screwRust = 0.0f;
                const float screwCoords[4][2] = { { 26.0f, 56.0f }, { 26.0f, 306.0f }, { 934.0f, 56.0f }, { 934.0f, 306.0f } };
                for (int s = 0; s < 4; ++s)
                {
                    const float dx = (float)x - screwCoords[s][0];
                    const float dy = (float)y - screwCoords[s][1];
                    const float d  = std::sqrt(dx * dx + dy * dy);
                    if (d < 42.0f)
                        screwRust = std::max(screwRust, (1.0f - d / 42.0f) * 0.85f);
                }

                const float nCoarse = fbmNoise((float)x * 0.025f, (float)y * 0.025f);
                const float nFine   = valueNoise((float)x * 0.18f, (float)y * 0.18f);
                const float brush   = valueNoise((float)x * 0.04f, (float)y * 0.65f);

                // Base weathered aquamarine-blue steel color (#366074 -> #254656)
                const float vignette = 1.0f - 0.30f * ((nx - 0.5f) * (nx - 0.5f) * 1.8f + (ny - 0.5f) * (ny - 0.5f) * 1.5f);
                float r = (52.0f + 16.0f * nCoarse + 7.0f * (brush - 0.5f)) * vignette;
                float gr = (95.0f + 18.0f * nCoarse + 8.0f * (brush - 0.5f)) * vignette;
                float b = (116.0f + 20.0f * nCoarse + 9.0f * (brush - 0.5f)) * vignette;

                // Rust corrosion mask along edges, screws, and random pitted spots
                const float rustTrigger = (borderProximity * 0.78f + screwRust * 0.75f + (nCoarse - 0.32f) * 0.88f + (nFine - 0.5f) * 0.25f);
                if (rustTrigger > 0.48f)
                {
                    const float rustMix = std::clamp((rustTrigger - 0.48f) * 3.4f, 0.0f, 1.0f);
                    // Deep oxidized iron & burnt orange rust (#6b2d14 / #96421d / #38180b)
                    const float rustR = 68.0f + 78.0f * nFine;
                    const float rustG = 28.0f + 34.0f * nFine * nFine;
                    const float rustB = 14.0f + 14.0f * nFine;
                    r  = r * (1.0f - rustMix) + rustR * rustMix;
                    gr = gr * (1.0f - rustMix) + rustG * rustMix;
                    b  = b * (1.0f - rustMix) + rustB * rustMix;
                }

                weatheredRackImage.setPixelAt(x, y, juce::Colour(
                    (juce::uint8)std::clamp((int)r,  0, 255),
                    (juce::uint8)std::clamp((int)gr, 0, 255),
                    (juce::uint8)std::clamp((int)b,  0, 255),
                    (juce::uint8)255));
            }
        }

        // Draw realistic metal scratches, paint chips, and rack ear notches
        for (int sc = 0; sc < 145; ++sc)
        {
            const float sx = hash2D(sc * 11, 43) * (float)kWidth;
            const float sy = hash2D(sc * 23, 89) * (float)kRackHeight;
            const float angle = (hash2D(sc, 107) - 0.5f) * 1.4f;
            const float len = 8.0f + 42.0f * hash2D(sc * 7, 19);
            const float ex = sx + std::cos(angle) * len;
            const float ey = sy + std::sin(angle) * len;

            // Dark groove + raw steel scratch glint
            g.setColour(juce::Colour(0xff14222b).withAlpha(0.42f));
            g.drawLine(sx, sy + 1.0f, ex, ey + 1.0f, 1.1f);
            g.setColour(juce::Colour(0xff9cb8c8).withAlpha(0.30f));
            g.drawLine(sx, sy, ex, ey, 0.75f);
        }

        // Rack ear U-notches on left and right edges (matching hardware photo)
        g.setColour(juce::Colour(0xff0b0f12));
        g.fillRoundedRectangle(-8.0f, 76.0f,  26.0f, 22.0f, 9.0f);
        g.fillRoundedRectangle(-8.0f, 256.0f, 26.0f, 22.0f, 9.0f);
        g.fillRoundedRectangle((float)kWidth - 18.0f, 76.0f,  26.0f, 22.0f, 9.0f);
        g.fillRoundedRectangle((float)kWidth - 18.0f, 256.0f, 26.0f, 22.0f, 9.0f);

        // Rust rim around rack ear notches
        g.setColour(juce::Colour(0xff6b3016).withAlpha(0.75f));
        g.drawRoundedRectangle(-8.0f, 76.0f,  26.0f, 22.0f, 9.0f, 2.2f);
        g.drawRoundedRectangle(-8.0f, 256.0f, 26.0f, 22.0f, 9.0f, 2.2f);
        g.drawRoundedRectangle((float)kWidth - 18.0f, 76.0f,  26.0f, 22.0f, 9.0f, 2.2f);
        g.drawRoundedRectangle((float)kWidth - 18.0f, 256.0f, 26.0f, 22.0f, 9.0f, 2.2f);

        // 4 Heavily Weathered Rusted Slotted Corner Screws
        const float screwCoords[4][2] = { { 28.0f, 56.0f }, { 28.0f, 306.0f }, { 932.0f, 56.0f }, { 932.0f, 306.0f } };
        for (int s = 0; s < 4; ++s)
        {
            const float sx = screwCoords[s][0];
            const float sy = screwCoords[s][1];
            g.setColour(juce::Colours::black.withAlpha(0.65f));
            g.fillEllipse(sx - 9.5f, sy - 8.5f, 19.0f, 19.0f);

            juce::ColourGradient scGrad(juce::Colour(0xff8c5230), sx - 8.0f, sy - 8.0f,
                                        juce::Colour(0xff381e10), sx + 8.0f, sy + 8.0f, false);
            scGrad.addColour(0.5, juce::Colour(0xff633820));
            g.setGradientFill(scGrad);
            g.fillEllipse(sx - 8.5f, sy - 8.5f, 17.0f, 17.0f);
            g.setColour(juce::Colour(0xff1c0e07));
            g.drawEllipse(sx - 8.5f, sy - 8.5f, 17.0f, 17.0f, 1.2f);
            g.drawLine(sx - 6.0f, sy - 2.5f + (float)(s % 2) * 4.0f,
                       sx + 6.0f, sy + 2.5f - (float)(s % 2) * 4.0f, 2.2f);
        }
    }

    // 2. Pre-render Black Diamond-Mesh Perforated Metal Amplifier Grille Overlay
    blackGrilleMeshImage = juce::Image(juce::Image::ARGB, kWidth, kHudHeight, true);
    {
        juce::Graphics g(blackGrilleMeshImage);
        const float cellW = 14.0f;
        const float cellH = 9.0f;

        // Draw interlocking expanded-metal diamond mesh wires across the tube bay
        for (float y = -cellH; y < (float)kHudHeight + cellH; y += cellH)
        {
            for (float x = -cellW; x < (float)kWidth + cellW; x += cellW)
            {
                juce::Path diamond;
                diamond.startNewSubPath(x, y - cellH * 0.5f);
                diamond.lineTo(x + cellW * 0.5f, y);
                diamond.lineTo(x, y + cellH * 0.5f);
                diamond.lineTo(x - cellW * 0.5f, y);
                diamond.closeSubPath();

                // Dark shadow under wire
                g.setColour(juce::Colour(0xf2050608));
                g.strokePath(diamond, juce::PathStrokeType(3.3f), juce::AffineTransform::translation(0.8f, 1.2f));

                // Black powder-coated steel wire
                g.setColour(juce::Colour(0xf5181b1f));
                g.strokePath(diamond, juce::PathStrokeType(2.3f));

                // Metallic top-edge wire glint (reflecting studio light & warm tube glow)
                g.setColour(juce::Colour(0x5558626c));
                g.drawLine(x - cellW * 0.45f, y - 0.5f, x, y - cellH * 0.45f, 0.9f);
                g.drawLine(x, y - cellH * 0.45f, x + cellW * 0.45f, y - 0.5f, 0.9f);
            }
        }

        // Heavy black brushed-steel rack ears & frame around the amplifier grille
        g.setColour(juce::Colour(0xff181a1d));
        g.fillRect(0, 0, 36, kHudHeight);
        g.fillRect(kWidth - 36, 0, 36, kHudHeight);
        g.fillRect(0, 0, kWidth, 10);
        g.fillRect(0, kHudHeight - 10, kWidth, 10);

        // Rusted/worn edges on top rack ears
        g.setColour(juce::Colour(0xff5e361e).withAlpha(0.65f));
        g.drawRect(0, 0, kWidth, kHudHeight, 2);
        g.setColour(juce::Colour(0xff08090b));
        g.fillRoundedRectangle(8.0f, 18.0f, 18.0f, 12.0f, 5.0f);
        g.fillRoundedRectangle(8.0f, (float)kHudHeight - 32.0f, 18.0f, 12.0f, 5.0f);
        g.fillRoundedRectangle((float)kWidth - 26.0f, 18.0f, 18.0f, 12.0f, 5.0f);
        g.fillRoundedRectangle((float)kWidth - 26.0f, (float)kHudHeight - 32.0f, 18.0f, 12.0f, 5.0f);
    }
}

void PultecEQP1AAudioProcessorEditor::drawScratchedText(juce::Graphics& g, const juce::String& text,
                                                        int x, int y, int w, int h, float fontSize,
                                                        juce::Justification just)
{
    g.setFont(juce::Font(fontSize, juce::Font::bold));

    // 1. Deep oxidized dark groove carved into metal
    g.setColour(juce::Colour(0xff101c24).withAlpha(0.92f));
    g.drawText(text, x + 1, y + 1, w, h, just, false);

    // 2. Subtle rust/warmth halo around the etched groove
    g.setColour(juce::Colour(0xff5c3018).withAlpha(0.48f));
    g.drawText(text, x - 1, y, w, h, just, false);

    // 3. Raw scratched steel/aluminum core lettering
    g.setColour(juce::Colour(0xffe4ddd0).withAlpha(0.92f));
    g.drawText(text, x, y, w, h, just, false);

    // 4. Micro-scratches across the etched text for authentic hand-scratched look
    const int seed = x * 13 + y * 29 + text.length() * 7;
    g.setColour(juce::Colour(0xff305466).withAlpha(0.50f));
    for (int i = 0; i < 3; ++i)
    {
        const float sx = (float)x + (float)w * (0.25f + 0.5f * hash2D(seed, i));
        const float sy = (float)y + (float)h * 0.25f;
        g.drawLine(sx - 3.0f, sy, sx + 3.0f, sy + (float)h * 0.5f, 0.9f);
    }
}

void PultecEQP1AAudioProcessorEditor::drawTubeBayBehindGrille(juce::Graphics& g, juce::Rectangle<float> area)
{
    // Deep dark amplifier interior cavity behind the grille
    juce::ColourGradient cavityGrad(juce::Colour(0xff060709), area.getX(), area.getY(),
                                    juce::Colour(0xff140d08), area.getX(), area.getBottom(), false);
    g.setGradientFill(cavityGrad);
    g.fillRect(area);

    // Dynamic filament brightness driven by TUBE DRIVE + live THD harmonic activity
    const float driveNorm = std::clamp(((float)tubeDriveKnob.getValue() + 12.0f) / 30.0f, 0.1f, 1.0f);
    const float thdGlow   = std::clamp(audioProcessor.dspEngine.getHarmonicMeter(), 0.0f, 1.0f);
    const bool  isOn      = !powerSwitch.getToggleState();
    const float glowLevel = isOn ? std::clamp(0.58f + 0.30f * driveNorm + 0.35f * thdGlow, 0.0f, 1.0f) : 0.08f;

    // 4 Vacuum Tubes sitting in the center-left bay behind the black mesh (12AX7, 12AU7, 12AX7, 6X4)
    const float tubeX[4]   = { 368.0f, 434.0f, 502.0f, 568.0f };
    const float tubeW[4]   = { 44.0f,  48.0f,  44.0f,  46.0f  };
    const float tubeTop[4] = { 34.0f,  26.0f,  36.0f,  30.0f  };

    for (int t = 0; t < 4; ++t)
    {
        const float tx = tubeX[t];
        const float tw = tubeW[t];
        const float ty = area.getY() + tubeTop[t];
        const float th = area.getBottom() - ty - 6.0f;
        const float cx = tx + tw * 0.5f;

        // 1. Ambient orange/amber radiant glow behind each tube
        if (isOn)
        {
            juce::ColourGradient aura(juce::Colour(0xffff5e14).withAlpha(0.46f * glowLevel),
                                      cx, ty + th * 0.62f,
                                      juce::Colours::transparentBlack,
                                      cx + tw * 1.4f, ty + th * 0.62f, true);
            g.setGradientFill(aura);
            g.fillEllipse(cx - tw * 1.3f, ty, tw * 2.6f, th * 1.15f);
        }

        // 2. Glass envelope with silver getter dome at top
        juce::Rectangle<float> glass(tx, ty, tw, th);
        juce::ColourGradient glassGrad(juce::Colour(0xff525860).withAlpha(0.75f), cx, ty,
                                       juce::Colour(0xff1e1612).withAlpha(0.85f), cx, ty + th * 0.35f, false);
        g.setGradientFill(glassGrad);
        g.fillRoundedRectangle(glass, tw * 0.44f);

        // Internal dark grey metal anode plates & mica spacers
        g.setColour(juce::Colour(0xff23201e));
        g.fillRect(tx + tw * 0.22f, ty + 24.0f, tw * 0.56f, th - 36.0f);
        g.setColour(juce::Colour(0xff4a423c));
        g.drawHorizontalLine((int)(ty + 26.0f), tx + 6.0f, tx + tw - 6.0f);
        g.drawHorizontalLine((int)(ty + th - 18.0f), tx + 6.0f, tx + tw - 18.0f);

        // 3. Hot Glowing Orange/Amber Heater Filaments (Top & Bottom Cathode Glow)
        if (isOn)
        {
            for (float fy : { ty + 32.0f, ty + th * 0.58f, ty + th * 0.78f })
            {
                juce::ColourGradient filGrad(juce::Colour(0xfffff0b8).withAlpha(glowLevel), cx, fy,
                                             juce::Colour(0xffff4d00).withAlpha(0.0f), cx + tw * 0.55f, fy, true);
                g.setGradientFill(filGrad);
                g.fillEllipse(cx - tw * 0.42f, fy - 14.0f, tw * 0.84f, 28.0f);
            }
        }

        // Specular glass tube outline & highlight
        g.setColour(juce::Colour(0xff9aa4ae).withAlpha(0.38f));
        g.drawRoundedRectangle(glass, tw * 0.44f, 1.2f);
        g.setColour(juce::Colours::white.withAlpha(0.22f));
        g.drawLine(tx + 6.0f, ty + 16.0f, tx + 6.0f, ty + th - 14.0f, 1.5f);
    }

    // 4. Overlay the pre-rendered Black Diamond-Mesh Perforated Metal Grille ON TOP of the glowing tubes!
    g.drawImageAt(blackGrilleMeshImage, (int)area.getX(), (int)area.getY());
}

void PultecEQP1AAudioProcessorEditor::toggleHudExpanded()
{
    isHudExpanded = !isHudExpanded;
    expandHudButton.setButtonText(isHudExpanded
        ? "[-] CLOSE TUBE BAY"
        : "[+] EXPAND TUBE BAY");
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
        "Save EQP - 1 Ultra Preset", startDir, "*.pultec;*.xml");

    auto chooserFlags = juce::FileBrowserComponent::saveMode | juce::FileBrowserComponent::canSelectFiles
                      | juce::FileBrowserComponent::warnAboutOverwriting;

    activeFileChooser->launchAsync(chooserFlags, [this](const juce::FileChooser& fc)
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
        "Load EQP - 1 Ultra Preset", startDir, "*.pultec;*.xml");

    auto chooserFlags = juce::FileBrowserComponent::openMode | juce::FileBrowserComponent::canSelectFiles;

    activeFileChooser->launchAsync(chooserFlags, [this](const juce::FileChooser& fc)
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
            activeTooltipText = "OFF / ON (TRUE BYPASS): Switches the entire EQP - 1 Ultra unit between active processing (ON) and true bypass (OFF).";
        else if (comp == &inputKnob)
            activeTooltipText = "INPUT (-18..+18 dB): SHIFT+LMB to auto-calibrate optimal input level (knob stays freely adjustable afterwards!).";
        else if (comp == &tubeDriveKnob)
            activeTooltipText = "TUBE & IRON DRIVE: Drives the Triad HS-56/HS-29, Peerless S-217-D transformers and glowing 12AX7/12AU7 tubes.";
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

    const auto rotParams = slider.getRotaryParameters();
    const float startA = rotParams.startAngleRadians;
    const float endA   = rotParams.endAngleRadians;

    const int n = (int)labels.size();
    for (int i = 0; i < n; ++i)
    {
        const float t = (n > 1) ? (float)i / (float)(n - 1) : 0.5f;
        const float a = startA + t * (endA - startA) - juce::MathConstants<float>::halfPi;
        const float lx = cx + std::cos(a) * baseR;
        const float ly = cy + std::sin(a) * baseR;
        drawScratchedText(g, labels[(size_t)i], (int)(lx - 16.0f), (int)(ly - 7.0f), 32, 14, fontSize);
    }
}

void PultecEQP1AAudioProcessorEditor::drawEQCurveDisplay(juce::Graphics& g, juce::Rectangle<float> area)
{
    // Heavy cast-iron oscilloscope bezel mounted into the black tube grille (right side, matching mockup!)
    g.setColour(juce::Colours::black.withAlpha(0.75f));
    g.fillRoundedRectangle(area.expanded(3.0f), 8.0f);

    juce::ColourGradient bezelGrad(juce::Colour(0xff2c2824), area.getX(), area.getY(),
                                   juce::Colour(0xff141210), area.getX(), area.getBottom(), false);
    g.setGradientFill(bezelGrad);
    g.fillRoundedRectangle(area, 7.0f);
    g.setColour(juce::Colour(0xff6b4428));
    g.drawRoundedRectangle(area, 7.0f, 1.6f);

    const auto screen = area.reduced(6.0f);
    // Vintage olive-amber phosphor oscilloscope CRT screen
    juce::ColourGradient crtGrad(juce::Colour(0xff111912), screen.getX(), screen.getY(),
                                 juce::Colour(0xff192418), screen.getX(), screen.getBottom(), false);
    g.setGradientFill(crtGrad);
    g.fillRoundedRectangle(screen, 4.0f);

    const auto plot = screen.reduced(28.0f, 12.0f);

    const double freqs[] = { 20.0, 30.0, 60.0, 100.0, 200.0, 500.0, 1000.0, 2000.0, 5000.0, 10000.0, 20000.0 };
    const char* labels[] = { "20", "30", "60", "100", "200", "500", "1k", "2k", "5k", "10k", "20k" };

    g.setFont(juce::Font(8.5f, juce::Font::plain));
    for (int i = 0; i < 11; ++i)
    {
        const float normX = (float)(std::log10(freqs[i] / 20.0) / std::log10(20000.0 / 20.0));
        const float px = plot.getX() + normX * plot.getWidth();
        g.setColour(juce::Colour(0xff354a32).withAlpha(0.55f));
        g.drawVerticalLine((int)px, plot.getY(), plot.getBottom());
        g.setColour(juce::Colour(0xff9ab88a));
        g.drawText(labels[i], (int)px - 12, (int)plot.getBottom() + 1, 24, 10, juce::Justification::centred);
    }

    for (int db = -18; db <= 18; db += 6)
    {
        const float normY = 0.5f - (float)db / 36.0f;
        const float py = plot.getY() + normY * plot.getHeight();
        g.setColour(db == 0 ? juce::Colour(0xff5b7d54) : juce::Colour(0xff263624));
        g.drawHorizontalLine((int)py, plot.getX(), plot.getRight());
        g.setColour(juce::Colour(0xff9ab88a));
        g.drawText((db > 0 ? "+" : "") + juce::String(db), (int)screen.getX() + 2, (int)py - 5, 24, 10, juce::Justification::centredRight);
    }

    juce::Path eqPath;
    juce::Path fillPath;
    const int numPts = 180;
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

    juce::ColourGradient curveFill(juce::Colour(0xffb8e86b).withAlpha(0.28f), plot.getX(), plot.getY(),
                                   juce::Colour(0xffff9436).withAlpha(0.18f), plot.getX(), plot.getBottom(), false);
    g.setGradientFill(curveFill);
    g.fillPath(fillPath);

    g.setColour(juce::Colour(0xffd8ff88));
    g.strokePath(eqPath, juce::PathStrokeType(2.2f));
}

void PultecEQP1AAudioProcessorEditor::drawRackChassis(juce::Graphics& g, juce::Rectangle<float> area)
{
    const float ry = area.getY();

    // 1. Draw the cached high-res Weathered Aquamarine-Blue & Rusted Steel Chassis
    g.drawImageAt(weatheredRackImage, 0, (int)ry);

    // 2. Hand-Scratched / Etched Brand & Model Inscription on Left (matching mockup!)
    drawScratchedText(g, "AQUAMARINE PLUG", 34, (int)ry + 92,  168, 20, 13.5f, juce::Justification::centred);
    drawScratchedText(g, "EQP - 1 Ultra",   34, (int)ry + 116, 168, 18, 12.5f, juce::Justification::centred);
    g.setColour(juce::Colour(0xff14222b).withAlpha(0.65f));
    g.drawLine(52.0f, ry + 113.0f, 184.0f, ry + 113.0f, 1.1f);
    g.setColour(juce::Colour(0xffb8ccd8).withAlpha(0.45f));
    g.drawLine(52.0f, ry + 112.0f, 184.0f, ry + 112.0f, 0.8f);

    // 3. Hand-Scratched Top Row Labels: BOOST | ATTEN | BOOST | ATTEN | ATTEN SEL
    drawScratchedText(g, "BOOST",     lowBoostKnob.getX() - 10,          (int)ry + 14, 128, 18, 13.0f);
    drawScratchedText(g, "ATTEN",     lowAttenKnob.getX() - 10,          (int)ry + 14, 128, 18, 13.0f);
    drawScratchedText(g, "BOOST",     highBoostKnob.getX() - 10,         (int)ry + 14, 128, 18, 13.0f);
    drawScratchedText(g, "ATTEN",     highAttenKnob.getX() - 10,         (int)ry + 14, 128, 18, 13.0f);
    drawScratchedText(g, "ATTEN SEL", highAttenFreqSelector.getX() - 16, (int)ry + 14, 128, 18, 13.0f);

    // Mid-panel etched section callouts (matching mockup!)
    drawScratchedText(g, "LOW FREQUENCY",  140, (int)ry + 168, 160, 16, 11.5f);
    drawScratchedText(g, "HIGH FREQUENCY", 495, (int)ry + 168, 160, 16, 11.5f);
    drawScratchedText(g, "HIGH FREQUENCY", 748, (int)ry + 160, 152, 15, 10.5f);
    drawScratchedText(g, "ATTEN SEL",      748, (int)ry + 174, 152, 15, 10.5f);

    // 4. Hand-Scratched Bottom Row Labels: IN / OUT | CPS | BANDWIDTH | KCS | OFF / ON
    drawScratchedText(g, "IN",        eqInSwitch.getX() - 6,             eqInSwitch.getY() - 18, 60, 15, 11.5f);
    drawScratchedText(g, "OUT",       eqInSwitch.getX() - 6,             eqInSwitch.getBottom() + 2, 60, 15, 11.5f);

    drawScratchedText(g, "CPS",       lowFreqSelector.getX() - 10,       lowFreqSelector.getBottom() + 8, 116, 16, 12.5f);

    drawScratchedText(g, "BANDWIDTH", bandwidthKnob.getX() - 20,         bandwidthKnob.getY() - 32, 148, 16, 12.0f);
    drawScratchedText(g, "BANDWIDTH", bandwidthKnob.getX() - 20,         bandwidthKnob.getBottom() + 8, 148, 16, 12.0f);
    drawScratchedText(g, "SHARP",     bandwidthKnob.getX() - 24,         bandwidthKnob.getBottom() - 4, 52, 12, 9.0f);
    drawScratchedText(g, "BROAD",     bandwidthKnob.getRight() - 28,     bandwidthKnob.getBottom() - 4, 52, 12, 9.0f);

    drawScratchedText(g, "KCS",       highBoostFreqSelector.getX() - 10, highBoostFreqSelector.getBottom() + 8, 116, 16, 12.5f);

    drawScratchedText(g, "OFF",       powerSwitch.getX() - 6,            powerSwitch.getY() - 18, 60, 15, 11.5f);
    drawScratchedText(g, "ON",        powerSwitch.getX() - 6,            powerSwitch.getBottom() + 2, 60, 15, 11.5f);

    // 5. Hand-Scratched Radial Dial Numbers around all 8 rotary controls
    const std::vector<juce::String> scale0to10 = { "0", "1", "2", "3", "4", "5", "6", "7", "8", "9", "10" };
    drawRadialDialNumbers(g, lowBoostKnob,  scale0to10, 5.0f, 10.5f);
    drawRadialDialNumbers(g, lowAttenKnob,  scale0to10, 5.0f, 10.5f);
    drawRadialDialNumbers(g, highBoostKnob, scale0to10, 5.0f, 10.5f);
    drawRadialDialNumbers(g, highAttenKnob, scale0to10, 5.0f, 10.5f);
    drawRadialDialNumbers(g, bandwidthKnob, scale0to10, 5.0f, 10.5f);

    drawRadialDialNumbers(g, lowFreqSelector,       { "20", "30", "60", "100" },              7.0f, 11.0f);
    drawRadialDialNumbers(g, highBoostFreqSelector, { "3", "4", "5", "8", "10", "12", "16" }, 8.0f, 11.0f);
    drawRadialDialNumbers(g, highAttenFreqSelector, { "5", "10", "20" },                      7.0f, 11.0f);

    // 6. Red Faceted Ruby Jewel Pilot Lamp (bottom-right between KCS and OFF/ON switch, matching mockup!)
    const bool active = !powerSwitch.getToggleState();
    const float harmGlow = std::clamp(audioProcessor.dspEngine.getHarmonicMeter(), 0.0f, 1.0f);
    const juce::Rectangle<float> jewel(722.0f, ry + 236.0f, 42.0f, 42.0f);

    if (active)
    {
        juce::ColourGradient glow(juce::Colour(0xffff2812).withAlpha(0.72f + 0.28f * harmGlow),
                                  jewel.getCentreX(), jewel.getCentreY(),
                                  juce::Colours::transparentBlack,
                                  jewel.getCentreX() + 36.0f, jewel.getCentreY() + 36.0f, true);
        g.setGradientFill(glow);
        g.fillEllipse(jewel.expanded(16.0f));
    }

    // Tarnished silver/rust bezel around ruby lamp
    juce::ColourGradient ringGrad(juce::Colour(0xffd0c8b8), jewel.getX(), jewel.getY(),
                                  juce::Colour(0xff4a3c30), jewel.getRight(), jewel.getBottom(), false);
    g.setGradientFill(ringGrad);
    g.fillEllipse(jewel);
    g.setColour(juce::Colour(0xff221812));
    g.drawEllipse(jewel, 1.5f);

    const auto innerRuby = jewel.reduced(6.0f);
    juce::ColourGradient rubyGrad(active ? juce::Colour(0xffff4830).brighter(0.35f * harmGlow) : juce::Colour(0xff4a1512),
                                  innerRuby.getX() + 6.0f, innerRuby.getY() + 6.0f,
                                  active ? juce::Colour(0xff8f0c06) : juce::Colour(0xff1e0706),
                                  innerRuby.getRight(), innerRuby.getBottom(), false);
    g.setGradientFill(rubyGrad);
    g.fillEllipse(innerRuby);

    g.setColour(juce::Colours::white.withAlpha(active ? 0.65f : 0.18f));
    g.fillEllipse(innerRuby.getX() + 5.0f, innerRuby.getY() + 4.0f, 8.0f, 6.0f);
}

void PultecEQP1AAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff0b0d0f));

    if (isHudExpanded)
    {
        juce::Rectangle<float> topArea(0.0f, 0.0f, (float)getWidth(), (float)kHudHeight);

        // 1. Draw the Glowing Vacuum Tubes behind the Black Perforated Amplifier Grille!
        drawTubeBayBehindGrille(g, topArea);

        // 2. Left Industrial Metal Plate housing Preset Browser & Tube Trim Controls
        juce::Rectangle<float> leftPlate(44.0f, 18.0f, 326.0f, (float)kHudHeight - 36.0f);
        g.setColour(juce::Colour(0xd814110f));
        g.fillRoundedRectangle(leftPlate, 5.0f);
        g.setColour(juce::Colour(0xff784625));
        g.drawRoundedRectangle(leftPlate, 5.0f, 1.5f);

        drawScratchedText(g, "AQUAMARINE TUBE BAY & PRESETS", (int)leftPlate.getX() + 8, (int)leftPlate.getY() + 4, 310, 16, 11.0f);

        auto drawTrimLabel = [&](juce::Slider& s, const juce::String& name)
        {
            auto b = s.getBounds();
            drawScratchedText(g, name, b.getX() - 16, b.getY() - 14, b.getWidth() + 32, 12, 9.5f);
            g.setColour(juce::Colour(0xffffb868));
            g.setFont(juce::Font(10.0f, juce::Font::bold));
            g.drawText(juce::String(s.getValue(), 1) + " dB", b.getX() - 10, b.getBottom() - 2, b.getWidth() + 20, 12, juce::Justification::centred);
        };

        drawTrimLabel(inputKnob,     "INPUT (Shift+LMB)");
        drawTrimLabel(tubeDriveKnob, "TUBE DRIVE");
        drawTrimLabel(outputKnob,    "OUTPUT");

        // 3. Right Recessed Oscilloscope EQ Curve Display Window (mounted in the grille!)
        juce::Rectangle<float> scopeArea(618.0f, 22.0f, 294.0f, (float)kHudHeight - 44.0f);
        drawEQCurveDisplay(g, scopeArea);
    }

    const float rackY = isHudExpanded ? (float)kHudHeight : 0.0f;
    juce::Rectangle<float> rackArea(0.0f, rackY, (float)getWidth(), (float)kRackHeight);
    drawRackChassis(g, rackArea);

    // Shift+LMB Input Auto-Calibration Overlay Modal
    if (showCalibrationBanner || audioProcessor.dspEngine.isCalibrationActive())
    {
        auto modal = getLocalBounds().toFloat().withSizeKeepingCentre(470.0f, 118.0f);
        g.setColour(juce::Colour(0xf2120e0b));
        g.fillRoundedRectangle(modal, 6.0f);
        g.setColour(juce::Colour(0xffff9638));
        g.drawRoundedRectangle(modal, 6.0f, 2.0f);

        g.setColour(juce::Colour(0xfff5efe0));
        g.setFont(juce::Font(14.5f, juce::Font::bold));
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
            g.setColour(juce::Colour(0xffffb868));
            g.drawText("Listening & calibrating transformer headroom... " + juce::String((int)(prog * 100.0f)) + "%",
                       modal.removeFromTop(32.0f), juce::Justification::centred);
            auto bar = modal.withSizeKeepingCentre(360.0f, 14.0f);
            g.setColour(juce::Colour(0xff2a1e16));
            g.fillRoundedRectangle(bar, 4.0f);
            g.setColour(juce::Colour(0xffff8822));
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

    if (activeTooltipText.isNotEmpty())
    {
        auto tipRect = juce::Rectangle<float>(44.0f, (float)getHeight() - 24.0f, (float)getWidth() - 88.0f, 20.0f);
        g.setColour(juce::Colour(0xf0120e0c));
        g.fillRoundedRectangle(tipRect, 3.0f);
        g.setColour(juce::Colour(0xff9e5a2b));
        g.drawRoundedRectangle(tipRect, 3.0f, 1.0f);
        g.setColour(juce::Colour(0xffe8dcc8));
        g.setFont(juce::Font(11.0f, juce::Font::plain));
        g.drawText(activeTooltipText, tipRect.reduced(8.0f, 0.0f), juce::Justification::centred);
    }
}

void PultecEQP1AAudioProcessorEditor::resized()
{
    const int ry = isHudExpanded ? kHudHeight : 0;

    // English Expand / Collapse Tube Bay button on top-left bezel of the blue rack
    expandHudButton.setBounds(42, ry + 8, 162, 22);

    juce::Component* const hudComps[] = {
        &presetBox, &savePresetBtn, &loadPresetBtn,
        &routingBox, &oversamplingBox, &inputKnob, &tubeDriveKnob,
        &outputKnob, &autoGainButton, &analogHumButton
    };
    for (juce::Component* c : hudComps)
    {
        c->setVisible(isHudExpanded);
    }

    if (isHudExpanded)
    {
        const int lx = 54;
        presetBox.setBounds(lx, 42, 198, 24);
        savePresetBtn.setBounds(lx + 202, 42, 50, 24);
        loadPresetBtn.setBounds(lx + 256, 42, 50, 24);

        routingBox.setBounds(lx, 72, 150, 22);
        oversamplingBox.setBounds(lx + 156, 72, 150, 22);

        inputKnob.setBounds(lx + 6,   114, 58, 58);
        tubeDriveKnob.setBounds(lx + 80,  114, 58, 58);
        outputKnob.setBounds(lx + 154, 114, 58, 58);

        autoGainButton.setBounds(lx + 222, 114, 84, 26);
        analogHumButton.setBounds(lx + 222, 146, 84, 26);
    }

    // TOP ROW: [LOW BOOST]   [LOW ATTEN]      [HIGH BOOST]   [HIGH ATTEN]      [ATTEN SEL]
    const int topRowY = ry + 42;
    lowBoostKnob.setBounds         (208, topRowY, 110, 110);
    lowAttenKnob.setBounds         (346, topRowY, 110, 110);
    highBoostKnob.setBounds        (506, topRowY, 110, 110);
    highAttenKnob.setBounds        (644, topRowY, 110, 110);
    highAttenFreqSelector.setBounds(776, topRowY + 10, 94, 94);

    // BOTTOM ROW: [IN Toggle]   [LOW FREQ CPS]   [BANDWIDTH]   [HIGH FREQ KCS]   [OFF/ON Switch]
    const int botRowY = ry + 202;
    eqInSwitch.setBounds           (110, botRowY + 26, 48, 54);
    lowFreqSelector.setBounds      (274, botRowY + 6,  96, 96);
    bandwidthKnob.setBounds        (428, botRowY - 2, 110, 110);
    highBoostFreqSelector.setBounds(588, botRowY + 6,  96, 96);
    powerSwitch.setBounds          (806, botRowY + 26, 48, 54);
}
