#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class SpectrumAnalyzerAudioProcessorEditor : public juce::AudioProcessorEditor, private juce::Timer
{
public:
    explicit SpectrumAnalyzerAudioProcessorEditor(SpectrumAnalyzerAudioProcessor&);
    ~SpectrumAnalyzerAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    // Helpers de dibujo — separan responsabilidades en paint()
    void drawTimeWaveform(juce::Graphics& g, juce::Rectangle<float> area);
    void drawTimeGrid(juce::Graphics& g, juce::Rectangle<float> area);
    void drawVUMeter(juce::Graphics& g, juce::Rectangle<int> area);
    void drawGRMeter(juce::Graphics& g, juce::Rectangle<int> area);

    SpectrumAnalyzerAudioProcessor& audioProcessor;

    // Controles
    juce::Slider thresholdSlider, releaseSlider, outputGainSlider, ceilingSlider;
    juce::Label  thresholdLabel, releaseLabel, outputGainLabel, ceilingLabel;

    // Attachments
    juce::AudioProcessorValueTreeState::SliderAttachment thresholdAttach;
    juce::AudioProcessorValueTreeState::SliderAttachment releaseAttach;
    juce::AudioProcessorValueTreeState::SliderAttachment outputGainAttach;
    juce::AudioProcessorValueTreeState::SliderAttachment ceilingAttach;

    juce::TextButton freezeButton{ "FREEZE" };

    // Estado inercial de los VUMeters
    float vuLeft = 0.0f;
    float vuRight = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumAnalyzerAudioProcessorEditor)
};