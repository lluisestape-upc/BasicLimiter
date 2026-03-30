#pragma once
#include <JuceHeader.h>
#include <array>
#include <atomic>

class SpectrumAnalyzerAudioProcessor : public juce::AudioProcessor
{
public:
    SpectrumAnalyzerAudioProcessor();
    ~SpectrumAnalyzerAudioProcessor() override;

    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    // ---------------------------------------------------------------
    //  Visualizer Data (Time Domain) - Lock-free
    // ---------------------------------------------------------------
    static constexpr int visualizerBufferSize = 2048;
    std::array<float, visualizerBufferSize> inputHistory{};
    std::array<float, visualizerBufferSize> grHistory{};
    std::atomic<int> historyWriteIndex{ 0 };

    // ---------------------------------------------------------------
    //  Metas para la UI
    // ---------------------------------------------------------------
    std::atomic<bool> isFrozen{ false };
    float rmsLeft = 0.0f;
    float rmsRight = 0.0f;

    // gainReductionDb: valor instantáneo para el medidor
    float getGainReductionDb() const { return gainReductionDb.load(std::memory_order_relaxed); }

    // ---------------------------------------------------------------
    //  APVTS - Incluye el parámetro "ceiling"
    // ---------------------------------------------------------------
    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Overrides obligatorios
    const juce::String getName() const override { return "Time Limiter Brickwall"; }
    bool acceptsMidi()  const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int  getNumPrograms()   override { return 1; }
    int  getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

private:
    // Estado interno del DSP
    float  envelopeState = 0.0f;
    double currentSampleRate = 44100.0;
    std::atomic<float> gainReductionDb{ 0.0f };

    // Variables de Downsampling para la UI
    int samplesPerPixel = 400;
    int sampleCount = 0;
    float currentMaxInput = 0.0f;
    float currentMinGR = 1.0f;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumAnalyzerAudioProcessor)
};