#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

class SpectrumAnalyzerAudioProcessorEditor : public juce::AudioProcessorEditor,
    private juce::Timer
{
public:
    explicit SpectrumAnalyzerAudioProcessorEditor(SpectrumAnalyzerAudioProcessor&);
    ~SpectrumAnalyzerAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    // Helpers de dibujo — separan responsabilidades en paint()
    void drawSpectrumCurve(juce::Graphics& g, float* scope,
        juce::Rectangle<float> area,
        juce::Colour colour, bool filled);
    void drawThresholdLine(juce::Graphics& g, juce::Rectangle<float> area);
    void drawFrequencyGrid(juce::Graphics& g, juce::Rectangle<float> area);
    void drawVUMeter(juce::Graphics& g, juce::Rectangle<int>   area);
    void drawGRMeter(juce::Graphics& g, juce::Rectangle<int>   area);

    SpectrumAnalyzerAudioProcessor& audioProcessor;

    // -----------------------------------------------------------------
    //  Controles del Limiter
    // -----------------------------------------------------------------
    juce::Slider thresholdSlider, releaseSlider, outputGainSlider;
    juce::Label  thresholdLabel, releaseLabel, outputGainLabel;

    // Los Attachments vinculan slider <-> parámetro APVTS de forma automática
    // y bidireccional. Deben declararse DESPUÉS de los sliders.
    juce::AudioProcessorValueTreeState::SliderAttachment thresholdAttach;
    juce::AudioProcessorValueTreeState::SliderAttachment releaseAttach;
    juce::AudioProcessorValueTreeState::SliderAttachment outputGainAttach;

    juce::TextButton freezeButton{ "FREEZE" };

    // -----------------------------------------------------------------
    //  Datos del espectro (procesados en timerCallback, dibujados en paint)
    // -----------------------------------------------------------------
    static constexpr int scopeSize = 512;
    float scopeData[scopeSize]{}; // Pre-limiter  (cyan)
    float scopeDataPost[scopeSize]{}; // Post-limiter (verde)

    // -----------------------------------------------------------------
    //  Estado de los vumeters (inercia analógica)
    // -----------------------------------------------------------------
    float vuLeft = 0.0f;
    float vuRight = 0.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumAnalyzerAudioProcessorEditor)
};