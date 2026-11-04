#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "PultecEQP1ADSP.h"

class PultecEQP1AAudioProcessor : public juce::AudioProcessor
{
public:
    PultecEQP1AAudioProcessor();
    ~PultecEQP1AAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;

    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram(int index) override;
    const juce::String getProgramName(int index) override;
    void changeProgramName(int index, const juce::String& newName) override;

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    juce::AudioProcessorValueTreeState apvts;
    PultecDSP::PultecEQP1AMasterEngine dspEngine;

    void applyFactoryPreset(int presetIndex);
    juce::File getUserPresetsFolder() const;
    bool saveUserPresetToFile(const juce::File& file);
    bool loadUserPresetFromFile(const juce::File& file);

private:
    void syncDSPFromParameters();
    int currentPresetIndex = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PultecEQP1AAudioProcessor)
};
