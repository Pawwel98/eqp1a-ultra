#include "PluginProcessor.h"
#include "PluginEditor.h"

PultecEQP1AAudioProcessor::PultecEQP1AAudioProcessor()
    : AudioProcessor(BusesProperties()
                     .withInput("Input", juce::AudioChannelSet::stereo(), true)
                     .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      apvts(*this, nullptr, "PARAMETERS", createParameterLayout())
{
}

PultecEQP1AAudioProcessor::~PultecEQP1AAudioProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout PultecEQP1AAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "input_db", 1 }, "Input Gain",
        juce::NormalisableRange<float>(-18.0f, 18.0f, 0.1f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "output_db", 1 }, "Output Gain",
        juce::NormalisableRange<float>(-18.0f, 18.0f, 0.1f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "tube_drive_db", 1 }, "Tube & Transformer Drive",
        juce::NormalisableRange<float>(-12.0f, 18.0f, 0.1f), 2.5f));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ "low_freq", 1 }, "Low Frequency (CPS)",
        juce::StringArray{ "20 CPS", "30 CPS", "45 CPS", "60 CPS", "100 CPS", "150 CPS" }, 1));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "low_boost", 1 }, "Low Boost",
        juce::NormalisableRange<float>(0.0f, 10.0f, 0.01f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "low_atten", 1 }, "Low Atten",
        juce::NormalisableRange<float>(0.0f, 10.0f, 0.01f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ "high_boost_freq", 1 }, "High Boost Freq (KCS)",
        juce::StringArray{ "3 KCS", "4 KCS", "5 KCS", "8 KCS", "10 KCS", "12 KCS", "16 KCS", "20 KCS" }, 5));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "high_boost", 1 }, "High Boost",
        juce::NormalisableRange<float>(0.0f, 10.0f, 0.01f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "bandwidth", 1 }, "Bandwidth (Sharp-Broad)",
        juce::NormalisableRange<float>(0.0f, 10.0f, 0.01f), 5.0f));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ "high_atten_freq", 1 }, "High Atten Sel (KCS)",
        juce::StringArray{ "5 KCS", "10 KCS", "20 KCS" }, 2));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{ "high_atten", 1 }, "High Atten",
        juce::NormalisableRange<float>(0.0f, 10.0f, 0.01f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ "eq_in", 1 }, "EQ In (Filter Enable)", true));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ "bypass", 1 }, "True Bypass", false));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ "auto_gain", 1 }, "Auto Gain", false));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{ "analog_hum", 1 }, "60Hz Tube Hum", false));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ "routing_mode", 1 }, "Routing Mode",
        juce::StringArray{ "STEREO", "MID ONLY", "SIDE ONLY", "DUAL MONO (ANALOG)" }, 3));

    params.push_back(std::make_unique<juce::AudioParameterChoice>(
        juce::ParameterID{ "oversampling", 1 }, "Oversampling",
        juce::StringArray{ "1x (Zero Latency)", "2x (Studio HD)", "4x (Mastering Ultra)" }, 1));

    return { params.begin(), params.end() };
}

const juce::String PultecEQP1AAudioProcessor::getName() const { return JucePlugin_Name; }
bool PultecEQP1AAudioProcessor::acceptsMidi() const { return false; }
bool PultecEQP1AAudioProcessor::producesMidi() const { return false; }
bool PultecEQP1AAudioProcessor::isMidiEffect() const { return false; }
double PultecEQP1AAudioProcessor::getTailLengthSeconds() const { return 0.05; }

int PultecEQP1AAudioProcessor::getNumPrograms() { return 6; }
int PultecEQP1AAudioProcessor::getCurrentProgram() { return currentPresetIndex; }

void PultecEQP1AAudioProcessor::setCurrentProgram(int index)
{
    applyFactoryPreset(index);
}

const juce::String PultecEQP1AAudioProcessor::getProgramName(int index)
{
    switch (index)
    {
        case 0: return "1. Flat Tube & Iron Line Amp (Default)";
        case 1: return "2. Kick & Sub Low-End Trick (20 CPS)";
        case 2: return "3. Bass Guitar Tight & Heavy (30 CPS)";
        case 3: return "4. Lead Vocal Silk & Presence (5k + 12k)";
        case 4: return "5. Drum Bus Punch & Cymbal Silk";
        case 5: return "6. Mastering Analog Air & Glue";
        default: return "Preset";
    }
}

void PultecEQP1AAudioProcessor::changeProgramName(int, const juce::String&) {}

void PultecEQP1AAudioProcessor::applyFactoryPreset(int index)
{
    currentPresetIndex = std::clamp(index, 0, 5);
    auto setParam = [this](const char* id, float plainVal)
    {
        if (auto* p = apvts.getParameter(id))
            p->setValueNotifyingHost(p->convertTo0to1(plainVal));
    };

    switch (currentPresetIndex)
    {
        case 0: // Flat Tube & Iron Line Amp
            setParam("low_freq", 1.0f);       // 30 CPS
            setParam("low_boost", 0.0f);
            setParam("low_atten", 0.0f);
            setParam("high_boost_freq", 5.0f);// 12 KCS
            setParam("high_boost", 0.0f);
            setParam("bandwidth", 5.0f);
            setParam("high_atten_freq", 2.0f);// 20 KCS
            setParam("high_atten", 0.0f);
            setParam("tube_drive_db", 2.5f);
            setParam("routing_mode", 3.0f);   // Dual Mono Analog
            break;

        case 1: // Kick & Sub Low-End Trick (20 CPS)
            setParam("low_freq", 0.0f);       // 20 CPS
            setParam("low_boost", 4.2f);
            setParam("low_atten", 2.8f);
            setParam("high_boost_freq", 3.0f);// 8 KCS
            setParam("high_boost", 3.2f);
            setParam("bandwidth", 4.5f);
            setParam("high_atten_freq", 2.0f);// 20 KCS
            setParam("high_atten", 1.5f);
            setParam("tube_drive_db", 4.5f);
            setParam("routing_mode", 3.0f);
            break;

        case 2: // Bass Guitar Tight & Heavy (30 CPS)
            setParam("low_freq", 1.0f);       // 30 CPS
            setParam("low_boost", 4.5f);
            setParam("low_atten", 3.4f);
            setParam("high_boost_freq", 1.0f);// 4 KCS
            setParam("high_boost", 2.8f);
            setParam("bandwidth", 3.5f);
            setParam("high_atten_freq", 1.0f);// 10 KCS
            setParam("high_atten", 2.2f);
            setParam("tube_drive_db", 5.5f);
            setParam("routing_mode", 3.0f);
            break;

        case 3: // Lead Vocal Silk & Presence
            setParam("low_freq", 4.0f);       // 100 CPS
            setParam("low_boost", 1.6f);
            setParam("low_atten", 2.4f);
            setParam("high_boost_freq", 5.0f);// 12 KCS
            setParam("high_boost", 4.6f);
            setParam("bandwidth", 7.2f);      // Broad silky skirt
            setParam("high_atten_freq", 2.0f);// 20 KCS
            setParam("high_atten", 2.0f);
            setParam("tube_drive_db", 3.5f);
            setParam("routing_mode", 3.0f);
            break;

        case 4: // Drum Bus Punch & Cymbal Silk
            setParam("low_freq", 1.0f);       // 30 CPS
            setParam("low_boost", 3.4f);
            setParam("low_atten", 2.2f);
            setParam("high_boost_freq", 4.0f);// 10 KCS
            setParam("high_boost", 3.0f);
            setParam("bandwidth", 6.5f);
            setParam("high_atten_freq", 2.0f);// 20 KCS
            setParam("high_atten", 2.4f);
            setParam("tube_drive_db", 4.8f);
            setParam("routing_mode", 3.0f);
            break;

        case 5: // Mastering Analog Air & Glue
            setParam("low_freq", 0.0f);       // 20 CPS
            setParam("low_boost", 1.8f);
            setParam("low_atten", 1.1f);
            setParam("high_boost_freq", 6.0f);// 16 KCS
            setParam("high_boost", 2.6f);
            setParam("bandwidth", 8.0f);      // Very broad air
            setParam("high_atten_freq", 2.0f);// 20 KCS
            setParam("high_atten", 0.8f);
            setParam("tube_drive_db", 2.0f);
            setParam("routing_mode", 3.0f);
            break;
    }
}

void PultecEQP1AAudioProcessor::prepareToPlay(double sampleRate, int)
{
    const int osIdx = (int)*apvts.getRawParameterValue("oversampling");
    dspEngine.prepare(sampleRate, osIdx);
    syncDSPFromParameters();
}

void PultecEQP1AAudioProcessor::releaseResources()
{
    dspEngine.reset();
}

bool PultecEQP1AAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    const auto out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    if (out != layouts.getMainInputChannelSet())
        return false;
    return true;
}

void PultecEQP1AAudioProcessor::syncDSPFromParameters()
{
    dspEngine.updateParameters(
        apvts.getRawParameterValue("input_db")->load(),
        apvts.getRawParameterValue("output_db")->load(),
        apvts.getRawParameterValue("tube_drive_db")->load(),
        (int)apvts.getRawParameterValue("low_freq")->load(),
        apvts.getRawParameterValue("low_boost")->load(),
        apvts.getRawParameterValue("low_atten")->load(),
        (int)apvts.getRawParameterValue("high_boost_freq")->load(),
        apvts.getRawParameterValue("high_boost")->load(),
        apvts.getRawParameterValue("bandwidth")->load(),
        (int)apvts.getRawParameterValue("high_atten_freq")->load(),
        apvts.getRawParameterValue("high_atten")->load(),
        apvts.getRawParameterValue("eq_in")->load() > 0.5f,
        apvts.getRawParameterValue("bypass")->load() > 0.5f,
        apvts.getRawParameterValue("auto_gain")->load() > 0.5f,
        apvts.getRawParameterValue("analog_hum")->load() > 0.5f,
        (int)apvts.getRawParameterValue("routing_mode")->load(),
        (int)apvts.getRawParameterValue("oversampling")->load()
    );
}

void PultecEQP1AAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int totalNumInputChannels  = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();
    for (int i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear(i, 0, buffer.getNumSamples());

    syncDSPFromParameters();

    float* left  = buffer.getWritePointer(0);
    float* right = (totalNumInputChannels > 1) ? buffer.getWritePointer(1) : nullptr;

    dspEngine.processBlock(left, right, buffer.getNumSamples());

    // Apply recommended input gain automatically when Shift+LMB calibration finishes
    if (dspEngine.isCalibrationComplete())
    {
        if (auto* p = apvts.getParameter("input_db"))
        {
            const float recDb = dspEngine.getRecommendedInputDb();
            p->setValueNotifyingHost(p->convertTo0to1(recDb));
        }
    }
}

bool PultecEQP1AAudioProcessor::hasEditor() const { return true; }

juce::AudioProcessorEditor* PultecEQP1AAudioProcessor::createEditor()
{
    return new PultecEQP1AAudioProcessorEditor(*this);
}

void PultecEQP1AAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void PultecEQP1AAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes));
    if (xmlState != nullptr && xmlState->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PultecEQP1AAudioProcessor();
}
