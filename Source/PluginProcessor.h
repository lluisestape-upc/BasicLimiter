#pragma once
#include <JuceHeader.h>

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
    //  FFT � Pre-limiter (se�al de ENTRADA)
    // ---------------------------------------------------------------
    static constexpr int fftOrder = 11;         // 2^11 = 2048 muestras
    static constexpr int fftSize = 1 << fftOrder;

    void pushNextSampleIntoFifo(float sample) noexcept;
    void pushNextSampleIntoFifoPost(float sample) noexcept; // se�al de SALIDA

    // Acceso a datos FFT para el editor
    float* getFFTData() { return fftData; }
    float* getFFTDataPost() { return fftDataPost.data(); }

    bool getNextFFTBlockReady()     const { return nextFFTBlockReady; }
    bool getNextFFTBlockReadyPost() const { return nextFFTBlockReadyPost.load(); }

    void setNextFFTBlockReady(bool v) { nextFFTBlockReady = v; }
    void setNextFFTBlockReadyPost(bool v) { nextFFTBlockReadyPost.store(v); }

    juce::dsp::FFT& getFFT() { return forwardFFT; }
    juce::dsp::WindowingFunction<float>& getWindow() { return window; }

    // ---------------------------------------------------------------
    //  Waveform oscilloscope buffers (time-domain display)
    // ---------------------------------------------------------------
    static constexpr int waveformSize = 1 << 17;  // 131072 samples ≈ 3s at 44.1kHz
    const float* getWaveformPre()  const { return waveformPre.data(); }
    const float* getWaveformPost() const { return waveformPost.data(); }
    int getWaveformWritePos()      const { return waveformWritePos.load(); }

    // ---------------------------------------------------------------
    //  Estado del Limiter – accesible por la UI
    // ---------------------------------------------------------------
    // gainReductionDb: cu�ntos dB se est� reduciendo la se�al ahora mismo.
    // Es atomic porque el hilo de audio escribe y la UI lee.
    float getGainReductionDb() const { return gainReductionDb.load(); }

    // ---------------------------------------------------------------
    //  M�tricas para la UI
    // ---------------------------------------------------------------
    std::atomic<bool> isFrozen{ false };
    float rmsLeft = 0.0f;
    float rmsRight = 0.0f;

    // ---------------------------------------------------------------
    //  APVTS � Gestor de par�metros
    // ---------------------------------------------------------------
    // Debe ser p�blico para que el editor pueda crear los SliderAttachments
    juce::AudioProcessorValueTreeState apvts;
    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // ---------------------------------------------------------------
    //  Overrides obligatorios (sin comportamiento relevante aqu�)
    // ---------------------------------------------------------------
    const juce::String getName() const override { return "ESP-L1"; }
    bool acceptsMidi()  const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }
    int  getNumPrograms()   override { return 1; }
    int  getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    // Serializaci�n del estado (guardar/cargar preset con el proyecto del DAW)
    void getStateInformation(juce::MemoryBlock& destData) override
    {
        auto state = apvts.copyState();
        std::unique_ptr<juce::XmlElement> xml(state.createXml());
        copyXmlToBinary(*xml, destData);
    }
    void setStateInformation(const void* data, int sizeInBytes) override
    {
        std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
        if (xml && xml->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xml));
    }

private:
    juce::dsp::FFT forwardFFT;
    juce::dsp::WindowingFunction<float> window;

    // Pre-limiter FFT buffers
    float fifo[fftSize]{};
    float fftData[2 * fftSize]{};
    int   fifoIndex = 0;
    bool  nextFFTBlockReady = false;

    // Post-limiter FFT buffers
    // std::array en lugar de C-array para evitar errores de tama�o en memcpy
    std::array<float, fftSize>      fifoPost{};
    std::array<float, fftSize * 2>  fftDataPost{};
    int  fifoIndexPost = 0;
    std::atomic<bool> nextFFTBlockReadyPost{ false };

    // Estado interno del algoritmo de limiting
    float  envelopeState = 0.0f;   // Seguidor de envolvente (sample a sample)
    double currentSampleRate = 44100.0; // Necesario para calcular el coef. de release
    std::atomic<float> gainReductionDb{ 0.0f }; // GR actual en dB (leído por la UI)

    // Waveform ring buffers (written by audio thread, read by UI thread for display only)
    std::array<float, waveformSize> waveformPre{};
    std::array<float, waveformSize> waveformPost{};
    std::atomic<int> waveformWritePos{ 0 };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SpectrumAnalyzerAudioProcessor)
};