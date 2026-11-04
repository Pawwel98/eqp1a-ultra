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
    const auto bounds = juce::Rectangle<float>((float)x, (float)y, (float)width, (float)height).reduced(4.0f);
    const float cx = bounds.getCentreX();
    const float cy = bounds.getCentreY();
    const float radius = std::min(bounds.getWidth(), bounds.getHeight()) * 0.5f;
    const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);

    const int style = slider.getProperties().getWithDefault("knobStyle", (int)StyleFlutedSkirted);

    // Draw scale ticks around the knob
    const int numTicks = (style == StyleChickenHead) ? (int)slider.getMaximum() + 1 : 11;
    for (int i = 0; i < numTicks; ++i)
    {
        const float t = (numTicks > 1) ? (float)i / (float)(numTicks - 1) : 0.0f;
        const float tickAngle = rotaryStartAngle + t * (rotaryEndAngle - rotaryStartAngle) - juce::MathConstants<float>::halfPi;
        const float rInner = radius * 0.84f;
        const float rOuter = radius * 0.96f;
        g.setColour(juce::Colour(0xffe8e2d5).withAlpha(0.78f));
        g.drawLine(cx + std::cos(tickAngle) * rInner, cy + std::sin(tickAngle) * rInner,
                   cx + std::cos(tickAngle) * rOuter, cy + std::sin(tickAngle) * rOuter,
                   (i % 2 == 0) ? 1.8f : 1.1f);
    }

    // Soft drop shadow on the blue rack faceplate
    g.setColour(juce::Colours::black.withAlpha(0.55f));
    g.fillEllipse(cx - radius * 0.76f + 2.0f, cy - radius * 0.76f + 4.5f, radius * 1.52f, radius * 1.52f);

    if (style == StyleFlutedSkirted || style == StyleTrimKnob)
    {
        // Brushed aluminum skirt
        const float skirtR = radius * 0.76f;
        juce::ColourGradient skirtGrad(juce::Colour(0xffd8dce0), cx - skirtR, cy - skirtR,
                                       juce::Colour(0xff6e757c), cx + skirtR, cy + skirtR, false);
        skirtGrad.addColour(0.5, juce::Colour(0xffb4bac0));
        g.setGradientFill(skirtGrad);
        g.fillEllipse(cx - skirtR, cy - skirtR, skirtR * 2.0f, skirtR * 2.0f);

        g.setColour(juce::Colour(0xff23272b));
        g.drawEllipse(cx - skirtR, cy - skirtR, skirtR * 2.0f, skirtR * 2.0f, 1.5f);

        // Black bakelite fluted cap
        const float capR = radius * 0.58f;
        juce::ColourGradient capGrad(juce::Colour(0xff383c42), cx - capR * 0.6f, cy - capR * 0.6f,
                                     juce::Colour(0xff101215), cx + capR * 0.7f, cy + capR * 0.7f, false);
        g.setGradientFill(capGrad);
        g.fillEllipse(cx - capR, cy - capR, capR * 2.0f, capR * 2.0f);

        // Fluted grip ribs (12 ribs rotating with knob)
        g.setColour(juce::Colour(0xff08090b).withAlpha(0.7f));
        for (int r = 0; r < 12; ++r)
        {
            const float ribA = angle + (float)r * (juce::MathConstants<float>::twoPi / 12.0f) - juce::MathConstants<float>::halfPi;
            g.drawLine(cx + std::cos(ribA) * (capR * 0.82f), cy + std::sin(ribA) * (capR * 0.82f),
                       cx + std::cos(ribA) * (capR * 0.99f), cy + std::sin(ribA) * (capR * 0.99f), 2.0f);
        }

        // Engraved white/cream indicator line
        const float indA = angle - juce::MathConstants<float>::halfPi;
        g.setColour(juce::Colour(0xfff5efe0));
        g.drawLine(cx + std::cos(indA) * (capR * 0.18f), cy + std::sin(indA) * (capR * 0.18f),
                   cx + std::cos(indA) * (skirtR * 0.96f), cy + std::sin(indA) * (skirtR * 0.96f), 2.6f);
    }
    else // StyleChickenHead (Vintage Pultec bakelite pointer selector switch)
    {
        const float baseR = radius * 0.56f;
        g.setColour(juce::Colour(0xff14171a));
        g.fillEllipse(cx - baseR, cy - baseR, baseR * 2.0f, baseR * 2.0f);
        g.setColour(juce::Colour(0xff8c96a0));
        g.drawEllipse(cx - baseR, cy - baseR, baseR * 2.0f, baseR * 2.0f, 1.4f);

        juce::Path pointer;
        pointer.addTriangle(-baseR * 0.36f, baseR * 0.35f,
                             baseR * 0.36f, baseR * 0.35f,
                             0.0f, -radius * 0.82f);
        g.setColour(juce::Colour(0xff1c2026));
        g.fillPath(pointer, juce::AffineTransform::rotation(angle).translated(cx, cy));
        g.setColour(juce::Colour(0xff525a64));
        g.strokePath(pointer, juce::PathStrokeType(1.3f), juce::AffineTransform::rotation(angle).translated(cx, cy));

        // White pointer tip
        const float indA = angle - juce::MathConstants<float>::halfPi;
        g.setColour(juce::Colour(0xfff5efe0));
        g.drawLine(cx + std::cos(indA) * (baseR * 0.15f), cy + std::sin(indA) * (baseR * 0.15f),
                   cx + std::cos(indA) * (radius * 0.78f), cy + std::sin(indA) * (radius * 0.78f), 2.4f);
    }
}

void PultecRackLookAndFeel::drawToggleButton(juce::Graphics& g, juce::ToggleButton& button,
                                             bool isHighlighted, bool)
{
    auto r = button.getLocalBounds().toFloat().reduced(2.0f);
    const bool on = button.getToggleState();

    juce::Colour bg = on ? juce::Colour(0xffb83228) : juce::Colour(0xff1a242e);
    if (button.getButtonText() == "EQ IN" && on)
        bg = juce::Colour(0xff26734d);
    else if (button.getButtonText() == "AUTO GAIN" && on)
        bg = juce::Colour(0xff2b5b84);

    if (isHighlighted) bg = bg.brighter(0.12f);

    g.setColour(bg);
    g.fillRoundedRectangle(r, 5.0f);
    g.setColour(juce::Colour(0xff8fa1b3).withAlpha(0.7f));
    g.drawRoundedRectangle(r, 5.0f, 1.2f);

    // Small status LED dot on left of button
    const float ledR = 4.0f;
    g.setColour(on ? juce::Colour(0xffffdd66) : juce::Colour(0xff3a4652));
    g.fillEllipse(r.getX() + 8.0f, r.getCentreY() - ledR, ledR * 2.0f, ledR * 2.0f);

    g.setColour(juce::Colour(0xfff4efe4));
    g.setFont(juce::Font(11.5f, juce::Font::bold));
    g.drawText(button.getButtonText(), r.withTrimmedLeft(18.0f), juce::Justification::centred, false);
}

//==============================================================================
PultecEQP1AAudioProcessorEditor::PultecEQP1AAudioProcessorEditor(PultecEQP1AAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p)
{
    setLookAndFeel(&pultecLNF);
    setSize(980, 620);

    auto configDial = [this](juce::Slider& s, int style, double defVal)
    {
        s.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
        s.setTextBoxStyle(juce::Slider::NoTextBox, false, 0, 0);
        s.getProperties().set("knobStyle", style);
        s.setDoubleClickReturnValue(true, defVal);
        addAndMakeVisible(s);
    };

    configDial(lowBoostKnob,          PultecRackLookAndFeel::StyleFlutedSkirted, 0.0);
    configDial(lowAttenKnob,          PultecRackLookAndFeel::StyleFlutedSkirted, 0.0);
    configDial(lowFreqSelector,       PultecRackLookAndFeel::StyleChickenHead,   1.0);

    configDial(highBoostKnob,         PultecRackLookAndFeel::StyleFlutedSkirted, 0.0);
    configDial(bandwidthKnob,         PultecRackLookAndFeel::StyleFlutedSkirted, 5.0);
    configDial(highBoostFreqSelector, PultecRackLookAndFeel::StyleChickenHead,   5.0);

    configDial(highAttenKnob,         PultecRackLookAndFeel::StyleFlutedSkirted, 0.0);
    configDial(highAttenFreqSelector, PultecRackLookAndFeel::StyleChickenHead,   2.0);

    configDial(inputKnob,             PultecRackLookAndFeel::StyleTrimKnob,      0.0);
    configDial(tubeDriveKnob,         PultecRackLookAndFeel::StyleTrimKnob,      2.5);
    configDial(outputKnob,            PultecRackLookAndFeel::StyleTrimKnob,      0.0);

    inputKnob.onShiftClick = [this]()
    {
        audioProcessor.dspEngine.startInputCalibration();
        showCalibrationBanner = true;
        calibrationDoneHoldTicks = 0;
        repaint();
    };

    addAndMakeVisible(eqInButton);
    addAndMakeVisible(bypassButton);
    addAndMakeVisible(autoGainButton);
    addAndMakeVisible(analogHumButton);

    routingBox.addItemList({ "STEREO (L/R)", "MID ONLY (Center)", "SIDE ONLY (Width)", "DUAL MONO (Analog Tol)" }, 1);
    addAndMakeVisible(routingBox);

    oversamplingBox.addItemList({ "1x (Zero Latency)", "2x (Studio HD)", "4x (Mastering Ultra)" }, 1);
    addAndMakeVisible(oversamplingBox);

    for (int i = 0; i < audioProcessor.getNumPrograms(); ++i)
        presetBox.addItem(audioProcessor.getProgramName(i), i + 1);
    presetBox.setSelectedId(1, juce::dontSendNotification);
    presetBox.onChange = [this]()
    {
        audioProcessor.applyFactoryPreset(presetBox.getSelectedId() - 1);
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

    eqInAtt          = std::make_unique<ButtonAttachment>(apvts, "eq_in",           eqInButton);
    bypassAtt        = std::make_unique<ButtonAttachment>(apvts, "bypass",          bypassButton);
    autoGainAtt      = std::make_unique<ButtonAttachment>(apvts, "auto_gain",       autoGainButton);
    analogHumAtt     = std::make_unique<ButtonAttachment>(apvts, "analog_hum",      analogHumButton);

    routingAtt       = std::make_unique<ComboAttachment>(apvts, "routing_mode",     routingBox);
    oversamplingAtt  = std::make_unique<ComboAttachment>(apvts, "oversampling",     oversamplingBox);

    startTimerHz(30);
}

PultecEQP1AAudioProcessorEditor::~PultecEQP1AAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void PultecEQP1AAudioProcessorEditor::timerCallback()
{
    if (showCalibrationBanner && !audioProcessor.dspEngine.isCalibrationActive())
    {
        if (++calibrationDoneHoldTicks > 55) // Hold completion message for ~1.8s
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

    if (comp != nullptr && (juce::Time::getMillisecondCounter() - hoverStartMs) >= 600)
    {
        if (comp == &lowBoostKnob)
            activeTooltipText = "LOW BOOST (0-10): Passive RC low shelf (+13.5 dB max). Combine with LOW ATTEN for the famous Pultec Low-End Trick.";
        else if (comp == &lowAttenKnob)
            activeTooltipText = "LOW ATTEN (0-10): Passive RC low cut (-17.5 dB max) with turnover 0.6 octaves higher than Boost, carving mud at 120-300 Hz.";
        else if (comp == &lowFreqSelector)
            activeTooltipText = "LOW FREQUENCY (CPS): Selects ganged capacitor tap (20, 30, 45, 60, 100, 150 CPS) for both Low Boost and Low Atten.";
        else if (comp == &highBoostKnob)
            activeTooltipText = "HIGH BOOST (0-10): Multi-tapped toroidal RLC inductor bell boost (+18 dB max) with silky permalloy saturation.";
        else if (comp == &bandwidthKnob)
            activeTooltipText = "BANDWIDTH (0 Sharp - 10 Broad): Adjusts inductor tank damping resistor Q and peak height, just like the real hardware.";
        else if (comp == &highBoostFreqSelector)
            activeTooltipText = "HIGH FREQUENCY (KCS): Selects toroidal inductor tap (3, 4, 5, 8, 10, 12, 16, 20 KCS) for silky presence and air.";
        else if (comp == &highAttenKnob)
            activeTooltipText = "HIGH ATTEN (0-10): Independent passive RC high-shelf roll-off (-16 dB max) to tame harshness while keeping air.";
        else if (comp == &highAttenFreqSelector)
            activeTooltipText = "ATTEN SEL (KCS): Independent high-shelf turnover selector (5, 10, 20 KCS).";
        else if (comp == &inputKnob)
            activeTooltipText = "INPUT (-18..+18 dB): SHIFT+LMB to auto-calibrate optimal input level for the Triad HS-56 transformer.";
        else if (comp == &tubeDriveKnob)
            activeTooltipText = "TUBE & IRON DRIVE: Pushes the 3 transformers (Triad/Peerless) and 12AX7/12AU7 push-pull tubes for warm 2nd/3rd harmonics.";
        else if (comp == &outputKnob)
            activeTooltipText = "OUTPUT (-18..+18 dB): Final clean trim after the Peerless S-217-D output transformer.";
        else if (comp == &eqInButton)
            activeTooltipText = "EQ IN: Hardware filter switch. When OFF, bypasses the passive LCR filter while keeping the 3 transformers & tubes active!";
        else if (comp == &bypassButton)
            activeTooltipText = "TRUE BYPASS: Complete click-free bypass of the entire plugin.";
        else if (comp == &autoGainButton)
            activeTooltipText = "AUTO GAIN: Automatically matches output loudness to input RMS so you judge tone, not volume.";
        else
            activeTooltipText.clear();
    }
}

void PultecEQP1AAudioProcessorEditor::drawEQCurveDisplay(juce::Graphics& g, juce::Rectangle<float> area)
{
    // Glass CRT / Analyzer Bezel
    juce::ColourGradient crtGrad(juce::Colour(0xff0b131b), area.getX(), area.getY(),
                                 juce::Colour(0xff111e2b), area.getX(), area.getBottom(), false);
    g.setGradientFill(crtGrad);
    g.fillRoundedRectangle(area, 6.0f);

    g.setColour(juce::Colour(0xff496580));
    g.drawRoundedRectangle(area, 6.0f, 1.5f);

    const auto plot = area.reduced(36.0f, 14.0f);

    // Frequency grid lines (20Hz .. 20kHz logarithmic)
    const double freqs[] = { 20.0, 30.0, 60.0, 100.0, 200.0, 500.0, 1000.0, 2000.0, 5000.0, 10000.0, 20000.0 };
    const char* labels[] = { "20", "30", "60", "100", "200", "500", "1k", "2k", "5k", "10k", "20k" };

    g.setFont(juce::Font(10.0f, juce::Font::plain));
    for (int i = 0; i < 11; ++i)
    {
        const float normX = (float)(std::log10(freqs[i] / 20.0) / std::log10(20000.0 / 20.0));
        const float px = plot.getX() + normX * plot.getWidth();
        g.setColour(juce::Colour(0xff253b50).withAlpha(0.65f));
        g.drawVerticalLine((int)px, plot.getY(), plot.getBottom());
        g.setColour(juce::Colour(0xff8ea8c2));
        g.drawText(labels[i], (int)px - 16, (int)plot.getBottom() + 1, 32, 12, juce::Justification::centred);
    }

    // dB horizontal lines (-18 dB to +18 dB)
    for (int db = -18; db <= 18; db += 6)
    {
        const float normY = 0.5f - (float)db / 38.0f;
        const float py = plot.getY() + normY * plot.getHeight();
        g.setColour(db == 0 ? juce::Colour(0xff4b6e8c) : juce::Colour(0xff1e3142));
        g.drawHorizontalLine((int)py, plot.getX(), plot.getRight());
        g.setColour(juce::Colour(0xff8ea8c2));
        g.drawText((db > 0 ? "+" : "") + juce::String(db), (int)area.getX() + 4, (int)py - 6, 28, 12, juce::Justification::centredRight);
    }

    // Trace exact passive LCR response curve
    juce::Path eqPath;
    juce::Path fillPath;
    const int numPts = 220;
    const float zeroY = plot.getCentreY();

    for (int i = 0; i <= numPts; ++i)
    {
        const float t = (float)i / (float)numPts;
        const double f = 20.0 * std::pow(1000.0, (double)t);
        const double magDb = std::clamp(audioProcessor.dspEngine.getMagnitudeDbAtFrequency(f), -19.0, 19.0);
        const float px = plot.getX() + t * plot.getWidth();
        const float py = plot.getY() + (0.5f - (float)magDb / 38.0f) * plot.getHeight();

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

    juce::ColourGradient curveFill(juce::Colour(0xff42b0ff).withAlpha(0.28f), plot.getX(), plot.getY(),
                                   juce::Colour(0xffff9436).withAlpha(0.18f), plot.getX(), plot.getBottom(), false);
    g.setGradientFill(curveFill);
    g.fillPath(fillPath);

    g.setColour(juce::Colour(0xff5ec2ff));
    g.strokePath(eqPath, juce::PathStrokeType(2.3f));

    // Real-time Tube & Transformer Harmonic Saturation Bar on right of analyzer
    const float harm = std::clamp(audioProcessor.dspEngine.getHarmonicMeter(), 0.0f, 1.0f);
    juce::Rectangle<float> thdBox(area.getRight() - 28.0f, plot.getY(), 18.0f, plot.getHeight());
    g.setColour(juce::Colour(0xff101820));
    g.fillRoundedRectangle(thdBox, 3.0f);
    const float fillH = thdBox.getHeight() * harm;
    juce::ColourGradient thdGrad(juce::Colour(0xffff5522), thdBox.getX(), thdBox.getY(),
                                 juce::Colour(0xffffb833), thdBox.getX(), thdBox.getBottom(), false);
    g.setGradientFill(thdGrad);
    g.fillRoundedRectangle(thdBox.getX(), thdBox.getBottom() - fillH, thdBox.getWidth(), fillH, 3.0f);
    g.setColour(juce::Colour(0xff8ea8c2));
    g.setFont(juce::Font(9.5f, juce::Font::bold));
    g.drawText("THD", (int)thdBox.getX() - 4, (int)plot.getBottom() + 1, 26, 12, juce::Justification::centred);
}

void PultecEQP1AAudioProcessorEditor::drawRackChassis(juce::Graphics& g, juce::Rectangle<float> area)
{
    // Classic Pultec EQP-1A3 Vintage Blue-Grey Hammered Enamel Faceplate
    juce::ColourGradient chassisGrad(juce::Colour(0xff3c5368), area.getX(), area.getY(),
                                     juce::Colour(0xff273949), area.getX(), area.getBottom(), false);
    chassisGrad.addColour(0.45, juce::Colour(0xff33485b));
    g.setGradientFill(chassisGrad);
    g.fillRoundedRectangle(area, 8.0f);

    // Brushed rack ears on left and right
    auto leftEar  = area.removeFromLeft(34.0f);
    auto rightEar = area.removeFromRight(34.0f);
    g.setColour(juce::Colour(0xff202d3a));
    g.fillRoundedRectangle(leftEar, 6.0f);
    g.fillRoundedRectangle(rightEar, 6.0f);

    // Rack mounting oval screw slots
    g.setColour(juce::Colour(0xff0e141a));
    for (float ry : { leftEar.getY() + 26.0f, leftEar.getBottom() - 38.0f })
    {
        g.fillRoundedRectangle(leftEar.getX() + 8.0f,  ry, 18.0f, 12.0f, 5.0f);
        g.fillRoundedRectangle(rightEar.getX() + 8.0f, ry, 18.0f, 12.0f, 5.0f);
    }

    // Section framing lines on faceplate
    g.setColour(juce::Colour(0xffe8e2d5).withAlpha(0.32f));
    g.drawRoundedRectangle(area.reduced(8.0f), 5.0f, 1.4f);

    // Vertical section dividers: LOW FREQUENCY | HIGH FREQUENCY BOOST | HIGH ATTEN | MASTER/TUBE
    const float yTop = area.getY() + 18.0f;
    const float yBot = area.getBottom() - 18.0f;
    g.drawLine(308.0f, yTop, 308.0f, yBot, 1.2f);
    g.drawLine(616.0f, yTop, 616.0f, yBot, 1.2f);
    g.drawLine(795.0f, yTop, 795.0f, yBot, 1.2f);
}

void PultecEQP1AAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff10161d));

    // 1. Top Mastering Header & Analyzer Bar
    juce::Rectangle<float> topArea(12.0f, 10.0f, (float)getWidth() - 24.0f, 175.0f);
    drawEQCurveDisplay(g, topArea.withTrimmedRight(255.0f));

    // Right side of Top Bar: Title Badge, Presets, Routing, Oversampling
    juce::Rectangle<float> ctrlBox(topArea.getRight() - 245.0f, topArea.getY(), 245.0f, topArea.getHeight());
    g.setColour(juce::Colour(0xff16212c));
    g.fillRoundedRectangle(ctrlBox, 6.0f);
    g.setColour(juce::Colour(0xff496580));
    g.drawRoundedRectangle(ctrlBox, 6.0f, 1.4f);

    g.setColour(juce::Colour(0xfff5efe0));
    g.setFont(juce::Font(15.5f, juce::Font::bold));
    g.drawText("PULTEC EQP-1A ULTRA", (int)ctrlBox.getX() + 10, (int)ctrlBox.getY() + 8, 225, 20, juce::Justification::centred);
    g.setColour(juce::Colour(0xff9bb4cc));
    g.setFont(juce::Font(10.5f, juce::Font::plain));
    g.drawText("PASSIVE LCR + TRIAD/PEERLESS + 12AX7/12AU7", (int)ctrlBox.getX() + 8, (int)ctrlBox.getY() + 26, 229, 14, juce::Justification::centred);

    // 2. Main Vintage Pultec 3U Rack Faceplate
    juce::Rectangle<float> rackArea(12.0f, 195.0f, (float)getWidth() - 24.0f, 412.0f);
    drawRackChassis(g, rackArea);

    // Section Titles on Blue Faceplate
    g.setColour(juce::Colour(0xfff5efe0));
    g.setFont(juce::Font(13.0f, juce::Font::bold));
    g.drawText("LOW FREQUENCY SECTION (SHELF)",     52,  208, 248, 20, juce::Justification::centred);
    g.drawText("HIGH FREQUENCY RESONANT BOOST",     316, 208, 292, 20, juce::Justification::centred);
    g.drawText("HIGH ATTEN SHELF",                  622, 208, 168, 20, juce::Justification::centred);
    g.drawText("TUBE & I/O STAGE",                  800, 208, 130, 20, juce::Justification::centred);

    // Knob Labels & Live Readouts
    auto drawKnobCaption = [&](juce::Slider& s, const juce::String& title, const juce::String& unitVal)
    {
        auto b = s.getBounds();
        g.setColour(juce::Colour(0xfff5efe0));
        g.setFont(juce::Font(11.5f, juce::Font::bold));
        g.drawText(title, b.getX() - 12, b.getY() - 18, b.getWidth() + 24, 16, juce::Justification::centred);

        g.setColour(juce::Colour(0xffd0e2f2));
        g.setFont(juce::Font(11.0f, juce::Font::plain));
        g.drawText(unitVal, b.getX() - 12, b.getBottom() - 2, b.getWidth() + 24, 15, juce::Justification::centred);
    };

    const char* lowCpsNames[]  = { "20 CPS", "30 CPS", "45 CPS", "60 CPS", "100 CPS", "150 CPS" };
    const char* highKcsNames[] = { "3 KCS", "4 KCS", "5 KCS", "8 KCS", "10 KCS", "12 KCS", "16 KCS", "20 KCS" };
    const char* attKcsNames[]  = { "5 KCS", "10 KCS", "20 KCS" };

    const int lowIdx  = std::clamp((int)std::round(lowFreqSelector.getValue()), 0, 5);
    const int hbIdx   = std::clamp((int)std::round(highBoostFreqSelector.getValue()), 0, 7);
    const int haIdx   = std::clamp((int)std::round(highAttenFreqSelector.getValue()), 0, 2);

    drawKnobCaption(lowBoostKnob,          "BOOST",     juce::String(lowBoostKnob.getValue(), 1));
    drawKnobCaption(lowAttenKnob,          "ATTEN",     juce::String(lowAttenKnob.getValue(), 1));
    drawKnobCaption(lowFreqSelector,       "LOW CPS",   lowCpsNames[lowIdx]);

    drawKnobCaption(highBoostKnob,         "BOOST",     juce::String(highBoostKnob.getValue(), 1));
    drawKnobCaption(bandwidthKnob,         "BANDWIDTH", juce::String(bandwidthKnob.getValue(), 1));
    drawKnobCaption(highBoostFreqSelector, "HIGH KCS",  highKcsNames[hbIdx]);

    drawKnobCaption(highAttenKnob,         "ATTEN",     juce::String(highAttenKnob.getValue(), 1));
    drawKnobCaption(highAttenFreqSelector, "ATTEN SEL", attKcsNames[haIdx]);

    drawKnobCaption(inputKnob,             "INPUT (Shift+Click)", juce::String(inputKnob.getValue(), 1) + " dB");
    drawKnobCaption(tubeDriveKnob,         "TUBE DRIVE",          juce::String(tubeDriveKnob.getValue(), 1) + " dB");
    drawKnobCaption(outputKnob,            "OUTPUT",              juce::String(outputKnob.getValue(), 1) + " dB");

    // Red Ruby Jewel Pilot Lamp (glows dynamically with 12AX7/12AU7 tube saturation!)
    const bool active = !bypassButton.getToggleState();
    const float harmGlow = std::clamp(audioProcessor.dspEngine.getHarmonicMeter(), 0.0f, 1.0f);
    const juce::Rectangle<float> jewel(848.0f, 552.0f, 32.0f, 32.0f);

    if (active)
    {
        juce::ColourGradient glow(juce::Colour(0xffff3b1f).withAlpha(0.75f + 0.25f * harmGlow),
                                  jewel.getCentreX(), jewel.getCentreY(),
                                  juce::Colours::transparentBlack,
                                  jewel.getCentreX() + 28.0f, jewel.getCentreY() + 28.0f, true);
        g.setGradientFill(glow);
        g.fillEllipse(jewel.expanded(12.0f));
    }

    g.setColour(juce::Colour(0xffa8b2bc));
    g.fillEllipse(jewel);
    g.setColour(active ? juce::Colour(0xffe62217).brighter(0.35f * harmGlow) : juce::Colour(0xff381210));
    g.fillEllipse(jewel.reduced(4.5f));
    g.setColour(juce::Colours::white.withAlpha(active ? 0.55f : 0.15f));
    g.fillEllipse(jewel.getX() + 8.0f, jewel.getY() + 7.0f, 8.0f, 6.0f);

    // Shift+LMB Input Auto-Calibration Overlay Modal
    if (showCalibrationBanner || audioProcessor.dspEngine.isCalibrationActive())
    {
        auto modal = getLocalBounds().toFloat().withSizeKeepingCentre(470.0f, 118.0f);
        g.setColour(juce::Colour(0xee0d151e));
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
                       + juce::String(audioProcessor.dspEngine.getRecommendedInputDb(), 1) + " dB",
                       modal, juce::Justification::centred);
        }
    }

    // Hover Tooltip Bar at bottom
    if (activeTooltipText.isNotEmpty())
    {
        auto tipRect = juce::Rectangle<float>(48.0f, (float)getHeight() - 32.0f, (float)getWidth() - 96.0f, 24.0f);
        g.setColour(juce::Colour(0xee0f1822));
        g.fillRoundedRectangle(tipRect, 5.0f);
        g.setColour(juce::Colour(0xff5ec2ff).withAlpha(0.75f));
        g.drawRoundedRectangle(tipRect, 5.0f, 1.1f);
        g.setColour(juce::Colour(0xfff5efe0));
        g.setFont(juce::Font(12.0f, juce::Font::plain));
        g.drawText(activeTooltipText, tipRect.reduced(10.0f, 0.0f), juce::Justification::centred);
    }
}

void PultecEQP1AAudioProcessorEditor::resized()
{
    // Top Right Header Controls
    const int rx = getWidth() - 245;
    presetBox.setBounds(rx + 12, 54, 220, 24);
    routingBox.setBounds(rx + 12, 84, 220, 24);
    oversamplingBox.setBounds(rx + 12, 114, 220, 24);

    eqInButton.setBounds(rx + 12, 146, 106, 28);
    autoGainButton.setBounds(rx + 126, 146, 106, 28);

    // 1. Low Frequency Section (Left)
    lowBoostKnob.setBounds(64, 252, 108, 108);
    lowAttenKnob.setBounds(184, 252, 108, 108);
    lowFreqSelector.setBounds(126, 404, 104, 104);

    // 2. High Frequency Resonant Boost Section (Center)
    highBoostKnob.setBounds(332, 252, 114, 114);
    bandwidthKnob.setBounds(474, 252, 114, 114);
    highBoostFreqSelector.setBounds(405, 404, 108, 108);

    // 3. High Attenuation Section (Center-Right)
    highAttenKnob.setBounds(650, 252, 108, 108);
    highAttenFreqSelector.setBounds(652, 404, 104, 104);

    // 4. Master Tube & I/O Column (Right)
    inputKnob.setBounds(825, 242, 78, 78);
    tubeDriveKnob.setBounds(825, 344, 78, 78);
    outputKnob.setBounds(825, 446, 78, 78);

    // Bottom Rack Toggles
    analogHumButton.setBounds(64, 548, 118, 30);
    bypassButton.setBounds(196, 548, 118, 30);
}
