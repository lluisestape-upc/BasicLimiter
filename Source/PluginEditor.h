#pragma once
#include <JuceHeader.h>
#include "PluginProcessor.h"

// =============================================================================
//  Theme definition — all colors that vary between themes
// =============================================================================
struct Theme
{
    const char* name;

    // Structure
    juce::Colour chassis;
    juce::Colour panelBg;
    juce::Colour headerBg;

    // Display areas
    juce::Colour waveBg;
    juce::Colour specBg;
    juce::Colour waveBorder;
    juce::Colour specBorder;
    juce::Colour panelBorder;

    // Text
    juce::Colour titleText;
    juce::Colour labelText;
    juce::Colour displayText;

    // Signals
    juce::Colour waveformPre;
    juce::Colour waveformPost;
    juce::Colour threshold;
    juce::Colour grBar;
    juce::Colour vuGreen;

    // Knob geometry colors
    juce::Colour knobRingA;
    juce::Colour knobRingB;
    juce::Colour knobBodyA;
    juce::Colour knobBodyB;
    juce::Colour knobArc;
    juce::Colour knobPointer;

    // Slider text-box
    juce::Colour tbBg;
    juce::Colour tbText;
};

// =============================================================================
//  VintageLookAndFeel — theme-aware custom look
// =============================================================================
class VintageLookAndFeel : public juce::LookAndFeel_V4
{
public:
    VintageLookAndFeel();
    void setTheme(const Theme& t);

    void drawRotarySlider(juce::Graphics&, int x, int y, int w, int h,
        float pos, float startAngle, float endAngle, juce::Slider&) override;

    void drawButtonBackground(juce::Graphics&, juce::Button&,
        const juce::Colour&, bool isOver, bool isDown) override;

    void drawButtonText(juce::Graphics&, juce::TextButton&,
        bool isOver, bool isDown) override;

private:
    const Theme* t = nullptr;   // non-owning; set via setTheme()
};

// =============================================================================
//  Editor
// =============================================================================
class SpectrumAnalyzerAudioProcessorEditor : public juce::AudioProcessorEditor,
    private juce::Timer
{
public:
    explicit SpectrumAnalyzerAudioProcessorEditor(SpectrumAnalyzerAudioProcessor&);
    ~SpectrumAnalyzerAudioProcessorEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

    static constexpr int kNumThemes = 3;
    static const Theme   kThemes[kNumThemes];

private:
    void timerCallback() override;
    void cycleTheme();

    void drawWaveform      (juce::Graphics&, juce::Rectangle<float>);
    void drawSpectrumCurve (juce::Graphics&, float* scope,
                            juce::Rectangle<float>, juce::Colour, bool filled);
    void drawThresholdLine (juce::Graphics&, juce::Rectangle<float>);
    void drawFrequencyGrid (juce::Graphics&, juce::Rectangle<float>);
    void drawVUMeter       (juce::Graphics&, juce::Rectangle<int>);
    void drawGRMeter       (juce::Graphics&, juce::Rectangle<int>);

    SpectrumAnalyzerAudioProcessor& audioProcessor;
    VintageLookAndFeel vintageLF;

    juce::Slider thresholdSlider, releaseSlider, outputGainSlider;
    juce::Label  thresholdLabel,  releaseLabel,  outputGainLabel;

    juce::AudioProcessorValueTreeState::SliderAttachment thresholdAttach;
    juce::AudioProcessorValueTreeState::SliderAttachment releaseAttach;
    juce::AudioProcessorValueTreeState::SliderAttachment outputGainAttach;

    juce::TextButton freezeButton{ "FREEZE" };
    juce::TextButton themeButton;

    static constexpr int scopeSize = 512;
    float scopeData    [scopeSize]{};
    float scopeDataPost[scopeSize]{};

    float vuLeft  = 0.0f;
    float vuRight = 0.0f;
    int   frozenWritePos = 0;
    int   themeIndex = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumAnalyzerAudioProcessorEditor)
};
