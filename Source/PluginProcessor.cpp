#include "PluginProcessor.h"
#include "PluginEditor.h"

SpectrumAnalyzerAudioProcessor::SpectrumAnalyzerAudioProcessor()
    : AudioProcessor(BusesProperties()
        .withInput("Input", juce::AudioChannelSet::stereo(), true)
        .withOutput("Output", juce::AudioChannelSet::stereo(), true)),
    apvts(*this, nullptr, "Parameters", createParameterLayout())
{
}

SpectrumAnalyzerAudioProcessor::~SpectrumAnalyzerAudioProcessor() {}

juce::AudioProcessorValueTreeState::ParameterLayout
SpectrumAnalyzerAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "threshold", "Threshold",
        juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f), 0.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "release", "Release",
        juce::NormalisableRange<float>(10.0f, 1000.0f, 1.0f, 0.5f), 100.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "outputGain", "Output Gain",
        juce::NormalisableRange<float>(-12.0f, 12.0f, 0.1f), 0.0f));

    // Parámetro de Ceiling: Brickwall max output
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        "ceiling", "Ceiling",
        juce::NormalisableRange<float>(-12.0f, 0.0f, 0.1f), -0.1f));

    return { params.begin(), params.end() };
}

void SpectrumAnalyzerAudioProcessor::prepareToPlay(double sampleRate, int /*samplesPerBlock*/)
{
    currentSampleRate = sampleRate;
    envelopeState = 0.0f;

    // Downsampling para la visualización (reducimos Hz de audio a Hz de píxel)
    samplesPerPixel = static_cast<int>(sampleRate / 100.0);
    sampleCount = 0;
    currentMaxInput = 0.0f;
    currentMinGR = 1.0f;

    inputHistory.fill(0.0f);
    grHistory.fill(1.0f);
    historyWriteIndex.store(0);
}

void SpectrumAnalyzerAudioProcessor::releaseResources() {}

void SpectrumAnalyzerAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const int numSamples = buffer.getNumSamples();
    const int numChannels = buffer.getNumChannels();

    // Medición RMS Pre-limiter para los vumetros
    rmsLeft = buffer.getRMSLevel(0, 0, numSamples);
    rmsRight = (numChannels > 1) ? buffer.getRMSLevel(1, 0, numSamples) : rmsLeft;

    // Obtener parámetros
    const float thresholdDb = apvts.getRawParameterValue("threshold")->load();
    const float releaseMs = apvts.getRawParameterValue("release")->load();
    const float outputGainDb = apvts.getRawParameterValue("outputGain")->load();
    const float ceilingDb = apvts.getRawParameterValue("ceiling")->load();

    const float thresholdLinear = juce::Decibels::decibelsToGain(thresholdDb);
    const float outputGain = juce::Decibels::decibelsToGain(outputGainDb);
    const float ceilingLinear = juce::Decibels::decibelsToGain(ceilingDb);

    // Coeficiente de release exponencial adaptado al SR
    const float releaseCoeff = std::exp(-1.0f / (static_cast<float>(currentSampleRate) * (releaseMs / 1000.0f)));

    float currentGR = 1.0f;
    bool frozen = isFrozen.load(std::memory_order_relaxed);

    for (int s = 0; s < numSamples; ++s)
    {
        // 1. Detección de picos (Linked Stereo)
        float peak = 0.0f;
        for (int ch = 0; ch < numChannels; ++ch)
            peak = std::max(peak, std::abs(buffer.getSample(ch, s)));

        // 2. Envolvente (Ataque 0ms, Release Exponencial)
        envelopeState = std::max(peak, envelopeState * releaseCoeff);

        // 3. Cálculo de GR
        currentGR = 1.0f;
        if (envelopeState > thresholdLinear && envelopeState > 0.0f)
            currentGR = thresholdLinear / envelopeState;

        // 4. Aplicar Proceso y Ceiling (Brickwall Clipper final)
        for (int ch = 0; ch < numChannels; ++ch)
        {
            float processed = buffer.getSample(ch, s) * currentGR * outputGain;

            // Hard-clipper de seguridad absoluto
            processed = juce::jlimit(-ceilingLinear, ceilingLinear, processed);

            buffer.setSample(ch, s, processed);
        }

        // 5. Lógica de Downsampling para el Visualizador (Lock-free)
        if (!frozen)
        {
            currentMaxInput = std::max(currentMaxInput, peak);
            currentMinGR = std::min(currentMinGR, currentGR);

            if (++sampleCount >= samplesPerPixel)
            {
                int idx = historyWriteIndex.load(std::memory_order_relaxed);
                inputHistory[idx] = currentMaxInput;
                grHistory[idx] = currentMinGR;

                // Avanzar índice circularmente
                idx = (idx + 1) % visualizerBufferSize;
                historyWriteIndex.store(idx, std::memory_order_release);

                // Reiniciar acumuladores para el siguiente píxel
                sampleCount = 0;
                currentMaxInput = 0.0f;
                currentMinGR = 1.0f;
            }
        }
    }

    // Actualizar GR Db para la UI (valor del último sample procesado)
    gainReductionDb.store(juce::Decibels::gainToDecibels(currentGR), std::memory_order_relaxed);
}

juce::AudioProcessorEditor* SpectrumAnalyzerAudioProcessor::createEditor()
{
    return new SpectrumAnalyzerAudioProcessorEditor(*this);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new SpectrumAnalyzerAudioProcessor();
}

void SpectrumAnalyzerAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void SpectrumAnalyzerAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml && xml->hasTagName(apvts.state.getType()))
        apvts.replaceState(juce::ValueTree::fromXml(*xml));
}