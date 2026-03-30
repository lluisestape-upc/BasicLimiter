#include "PluginProcessor.h"
#include "PluginEditor.h"

// =============================================================================
//  Constructor
// =============================================================================
SpectrumAnalyzerAudioProcessor::SpectrumAnalyzerAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
    forwardFFT(fftOrder),
    window(fftSize, juce::dsp::WindowingFunction<float>::hann),
    apvts(*this, nullptr, "Parameters", createParameterLayout())
    
{
}

SpectrumAnalyzerAudioProcessor::~SpectrumAnalyzerAudioProcessor() {}

// =============================================================================
//  Layout de parámetros (llamado UNA sola vez desde el constructor de apvts)
// =============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout
SpectrumAnalyzerAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Threshold: umbral a partir del cual el limiter actúa
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "threshold", "Threshold",
        juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f), 0.0f));

    // Release: tiempo (ms) que tarda la GR en volver a cero tras el pico
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "release", "Release",
        juce::NormalisableRange<float>(10.0f, 1000.0f, 1.0f, 0.5f), 100.0f));
    // El skew 0.5 hace que el knob no sea lineal: más resolución en valores bajos

    // Output Gain: ajuste de nivel tras el limiting
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "outputGain", "Output Gain",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f));

    return { params.begin(), params.end() };
}

// =============================================================================
//  prepareToPlay — Se llama cuando el DAW arranca o cambia la configuración
// =============================================================================
void SpectrumAnalyzerAudioProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;
    envelopeState = 0.0f;

    std::fill(std::begin(fifo), std::end(fifo), 0.0f);
    fifoPost.fill(0.0f);
    fifoIndex = 0;
    fifoIndexPost = 0;
    nextFFTBlockReady = false;
    nextFFTBlockReadyPost.store(false);
}

void SpectrumAnalyzerAudioProcessor::releaseResources() {}

// =============================================================================
//  FIFO Pre-limiter (señal de ENTRADA al analizador)
// =============================================================================
void SpectrumAnalyzerAudioProcessor::pushNextSampleIntoFifo(float sample) noexcept
{
    if (fifoIndex == fftSize)
    {
        if (!nextFFTBlockReady)
        {
            std::memcpy(fftData, fifo, sizeof(fifo));
            nextFFTBlockReady = true;
        }
        fifoIndex = 0;
    }
    fifo[fifoIndex++] = sample;
}

// =============================================================================
//  FIFO Post-limiter (señal de SALIDA tras el DSP)
//  Mismo mecanismo que el pre, pero con variables independientes para que
//  ambos espectros no interfieran entre sí.
// =============================================================================
void SpectrumAnalyzerAudioProcessor::pushNextSampleIntoFifoPost(float sample) noexcept
{
    if (fifoIndexPost == fftSize)
    {
        if (!nextFFTBlockReadyPost.load())
        {
            // Copia solo los primeros fftSize floats; la segunda mitad de fftDataPost
            // actúa como buffer scratch para la FFT y ya fue inicializada a 0.
            std::memcpy(fftDataPost.data(), fifoPost.data(),
                fifoPost.size() * sizeof(float));
            nextFFTBlockReadyPost.store(true);
        }
        fifoIndexPost = 0;
    }
    fifoPost[fifoIndexPost++] = sample;
}

// =============================================================================
//  processBlock — El corazón del plugin
// =============================================================================
void SpectrumAnalyzerAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
    juce::MidiBuffer& /*midiMessages*/)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    // ------------------------------------------------------------------
    //  1. RMS para los vumeters (pre-limiter)
    //     getRMSLevel es más apropiado que getMagnitude (peak) para un VU meter
    // ------------------------------------------------------------------
    rmsLeft = buffer.getRMSLevel(0, 0, numSamples);
    rmsRight = (numChannels > 1) ? buffer.getRMSLevel(1, 0, numSamples) : rmsLeft;

    // ------------------------------------------------------------------
    //  2. Alimentar el FFT de ENTRADA (antes de tocar el audio)
    // ------------------------------------------------------------------
    const auto* leftIn = buffer.getReadPointer(0);
    for (int i = 0; i < numSamples; ++i)
        pushNextSampleIntoFifo(leftIn[i]);

    // ------------------------------------------------------------------
    //  3. Leer parámetros del APVTS
    //     getRawParameterValue devuelve un std::atomic<float>, .load() es thread-safe
    // ------------------------------------------------------------------
    const float thresholdDb = apvts.getRawParameterValue("threshold")->load();
    const float releaseMs = apvts.getRawParameterValue("release")->load();
    const float outputGainDb = apvts.getRawParameterValue("outputGain")->load();

    const float thresholdLinear = juce::Decibels::decibelsToGain(thresholdDb);
    const float outputGain = juce::Decibels::decibelsToGain(outputGainDb);

    // Coeficiente de release: decaimiento exponencial adaptado al sample rate.
    // Fórmula: e^(-1 / (SR * T_release_en_segundos))
    // A 44100 Hz con 100ms: coeff =aprox 0.9997 (decae muy suavemente)
    const float releaseCoeff = std::exp(
        -1.0f / (static_cast<float>(currentSampleRate) * (releaseMs / 1000.0f)));

    // ------------------------------------------------------------------
    //  4. Algoritmo de Limiting — muestra a muestra
    //     Por qué muestra a muestra y no por bloque:
    //     El limiter necesita reaccionar instantáneamente a cada pico.
    //     Si procesáramos por bloques, podríamos clipear antes de actuar.
    // ------------------------------------------------------------------
    float currentGR = 1.0f; // Ganancia de reducción del último sample (para la UI)

    for (int s = 0; s < numSamples; ++s)
    {
        // Detectar el pico absoluto entre todos los canales
        float peak = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
            peak = std::max(peak, std::abs(buffer.getSample(ch, s)));

        // Seguidor de envolvente:
        // - Sube instantáneamente al pico (attack = 0, comportamiento de limiter)
        // - Decae exponencialmente con el coeficiente de release
        envelopeState = std::max(peak, envelopeState * releaseCoeff);

        // Calcular ganancia de reducción necesaria
        // Si la envolvente supera el umbral, reducimos proporcionalmente
        currentGR = 1.0f;
        if (envelopeState > thresholdLinear && envelopeState > 0.0f)
            currentGR = thresholdLinear / envelopeState;

        // Aplicar GR + Output Gain a todos los canales (linked stereo)
        for (int ch = 0; ch < numChannels; ++ch)
            buffer.setSample(ch, s, buffer.getSample(ch, s) * currentGR * outputGain);
    }

    // Actualizar el medidor de GR para la UI (valor del último sample del bloque)
    gainReductionDb.store(juce::Decibels::gainToDecibels(currentGR));

    // ------------------------------------------------------------------
    //  5. Alimentar el FFT de SALIDA (señal ya limitada)
    // ------------------------------------------------------------------
    const auto* leftOut = buffer.getReadPointer(0);
    for (int i = 0; i < numSamples; ++i)
        pushNextSampleIntoFifoPost(leftOut[i]);
}

// =============================================================================
juce::AudioProcessorEditor* SpectrumAnalyzerAudioProcessor::createEditor()
{
    return new SpectrumAnalyzerAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SpectrumAnalyzerAudioProcessor();
}