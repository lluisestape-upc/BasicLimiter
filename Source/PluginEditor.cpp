#include "PluginProcessor.h"
#include "PluginEditor.h"

// =============================================================================
//  Constructor
// =============================================================================
SpectrumAnalyzerAudioProcessorEditor::SpectrumAnalyzerAudioProcessorEditor(
    SpectrumAnalyzerAudioProcessor& p)
    : AudioProcessorEditor(&p),
    audioProcessor(p),
    // Los Attachments conectan slider <-> APVTS. Deben construirse AQUÍ,
    // no en el cuerpo del constructor, para que el binding sea inmediato.
    thresholdAttach(p.apvts, "threshold", thresholdSlider),
    releaseAttach(p.apvts, "release", releaseSlider),
    outputGainAttach(p.apvts, "outputGain", outputGainSlider)
{
    // Threshold
    thresholdSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    thresholdSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 18);
    thresholdSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xFFE63946));
    thresholdSlider.setTextValueSuffix(" dB");
    addAndMakeVisible(thresholdSlider);

    thresholdLabel.setText("Threshold", juce::dontSendNotification);
    thresholdLabel.setJustificationType(juce::Justification::centred);
    thresholdLabel.setFont(juce::Font(12.0f));
    addAndMakeVisible(thresholdLabel);

    // Release
    releaseSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    releaseSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 18);
    releaseSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xFFFFB703));
    releaseSlider.setTextValueSuffix(" ms");
    addAndMakeVisible(releaseSlider);

    releaseLabel.setText("Release", juce::dontSendNotification);
    releaseLabel.setJustificationType(juce::Justification::centred);
    releaseLabel.setFont(juce::Font(12.0f));
    addAndMakeVisible(releaseLabel);

    // Output Gain
    outputGainSlider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
    outputGainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 18);
    outputGainSlider.setColour(juce::Slider::rotarySliderFillColourId, juce::Colour(0xFF06D6A0));
    outputGainSlider.setTextValueSuffix(" dB");
    addAndMakeVisible(outputGainSlider);

    outputGainLabel.setText("Out Gain", juce::dontSendNotification);
    outputGainLabel.setJustificationType(juce::Justification::centred);
    outputGainLabel.setFont(juce::Font(12.0f));
    addAndMakeVisible(outputGainLabel);

    // Freeze
    freezeButton.setClickingTogglesState(true);
    freezeButton.onClick = [this]()
        {
            audioProcessor.isFrozen.store(freezeButton.getToggleState());
        };
    addAndMakeVisible(freezeButton);

    // 30 fps es suficiente para visualización de espectro; 60 fps es innecesario
    // y consume más CPU en el hilo de mensajes
    startTimerHz(30);
    setSize(900, 500);
}

SpectrumAnalyzerAudioProcessorEditor::~SpectrumAnalyzerAudioProcessorEditor()
{
    stopTimer();
}

// =============================================================================
//  timerCallback — Se ejecuta en el hilo de mensajes (UI thread)
//  Lee datos del procesador y actualiza los arrays de scope para paint()
// =============================================================================
void SpectrumAnalyzerAudioProcessorEditor::timerCallback()
{
    const double sampleRate = audioProcessor.getSampleRate();

    // ------------------------------------------------------------------
    //  VU meters con inercia analógica
    // ------------------------------------------------------------------
    const float dbL = juce::Decibels::gainToDecibels(audioProcessor.rmsLeft, -60.0f);
    const float dbR = juce::Decibels::gainToDecibels(audioProcessor.rmsRight, -60.0f);
    const float tL = juce::jlimit(0.0f, 1.0f, juce::jmap(dbL, -60.0f, 0.0f, 0.0f, 1.0f));
    const float tR = juce::jlimit(0.0f, 1.0f, juce::jmap(dbR, -60.0f, 0.0f, 0.0f, 1.0f));
    vuLeft = (tL > vuLeft) ? tL : juce::jmax(0.0f, vuLeft - 0.02f);
    vuRight = (tR > vuRight) ? tR : juce::jmax(0.0f, vuRight - 0.02f);

    const bool frozen = audioProcessor.isFrozen.load();

    auto processScopeData = [&](float* fftData, float* scope, auto readyGetter, auto readySetter)
        {
            if (frozen || !readyGetter()) return;

            audioProcessor.getWindow().multiplyWithWindowingTable(fftData, audioProcessor.fftSize);
            audioProcessor.getFFT().performFrequencyOnlyForwardTransform(fftData);

            for (int i = 0; i < scopeSize; ++i)
            {
                // Escala logarítmica en frecuencia: 20Hz -> 20kHz
                const float freq = 20.0f * std::pow(1000.0f, (float)i / (float)scopeSize);
                const int   fftIndex = juce::jlimit(0, audioProcessor.fftSize - 1,
                    (int)(freq * audioProcessor.fftSize / sampleRate));

                const float normMag = fftData[fftIndex] / (audioProcessor.fftSize * 0.5f);
                const float dbFS = juce::Decibels::gainToDecibels(normMag, -100.0f);
                const float level = juce::jlimit(0.0f, 1.0f,
                    juce::jmap(dbFS, -100.0f, 0.0f, 0.0f, 1.0f));

                // Peak-hold + decaimiento suave (efecto "aguja analógica")
                if (level >= scope[i]) scope[i] = level;
                else                   scope[i] = juce::jmax(0.0f, scope[i] - 0.035f);
            }
            readySetter(false);
        };

    // Pre-limiter
    processScopeData(audioProcessor.getFFTData(), scopeData,
        [this] { return audioProcessor.getNextFFTBlockReady(); },
        [this](bool v) { audioProcessor.setNextFFTBlockReady(v); });

    // Post-limiter
    processScopeData(audioProcessor.getFFTDataPost(), scopeDataPost,
        [this] { return audioProcessor.getNextFFTBlockReadyPost(); },
        [this](bool v) { audioProcessor.setNextFFTBlockReadyPost(v); });

    repaint();
}

// =============================================================================
//  paint — Todo el dibujo ocurre aquí, con datos ya calculados en timerCallback
// =============================================================================
void SpectrumAnalyzerAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xFF0E1116));

    auto bounds = getLocalBounds();
    auto controlsArea = bounds.removeFromBottom(130);
    auto metersArea = bounds.removeFromRight(100);
    auto spectrumArea = bounds; // lo que queda: área del espectro

    // ------------------------------------------------------------------
    //  1. Fondo del espectro
    // ------------------------------------------------------------------
    g.setColour(juce::Colour(0xFF080B0F));
    g.fillRect(spectrumArea);
    g.setColour(juce::Colour(0xFF1E2128));
    g.drawRect(spectrumArea, 1);

    // ------------------------------------------------------------------
    //  2. Grid de frecuencias y amplitud
    // ------------------------------------------------------------------
    drawFrequencyGrid(g, spectrumArea.toFloat());

    // ------------------------------------------------------------------
    //  3. Espectro PRE-limiter (cyan, relleno semitransparente)
    //     -> Muestra la señal tal como llega al plugin
    // ------------------------------------------------------------------
    drawSpectrumCurve(g, scopeData, spectrumArea.toFloat(),
        juce::Colour(0xFF00D4FF), true);

    // ------------------------------------------------------------------
    //  4. Espectro POST-limiter (verde, solo trazo)
    //     -> Muestra la señal tras el DSP. Si hay GR, verás que el
    //       espectro verde queda por debajo del cyan en los picos.
    // ------------------------------------------------------------------
    drawSpectrumCurve(g, scopeDataPost, spectrumArea.toFloat(),
        juce::Colour(0xFF06D6A0), false);

    // ------------------------------------------------------------------
    //  5. Línea de Threshold (roja)
    //     -> La referencia visual de dónde actúa el limiter
    // ------------------------------------------------------------------
    drawThresholdLine(g, spectrumArea.toFloat());

    // ------------------------------------------------------------------
    //  6. Medidores: VU + GR
    // ------------------------------------------------------------------
    auto vuArea = metersArea.removeFromLeft(60).reduced(6, 35);
    auto grArea = metersArea.reduced(4, 35);
    drawVUMeter(g, vuArea);
    drawGRMeter(g, grArea);

    // ------------------------------------------------------------------
    //  7. Fondo del área de controles
    // ------------------------------------------------------------------
    g.setColour(juce::Colour(0xFF12151A));
    g.fillRect(controlsArea);
    g.setColour(juce::Colour(0xFF2A2D35));
    g.drawLine((float)controlsArea.getX(), (float)controlsArea.getY(),
        (float)controlsArea.getRight(), (float)controlsArea.getY(), 1.0f);
}

// =============================================================================
//  Helpers de dibujo
// =============================================================================

void SpectrumAnalyzerAudioProcessorEditor::drawSpectrumCurve(
    juce::Graphics& g, float* scope, juce::Rectangle<float> area,
    juce::Colour colour, bool filled)
{
    const float aX = area.getX();
    const float aY = area.getY();
    const float aW = area.getWidth();
    const float aH = area.getHeight();

    // Construimos la curva Bézier con los puntos del scope
    juce::Path curve;
    juce::Array<juce::Point<float>> pts;
    pts.ensureStorageAllocated(scopeSize);

    for (int i = 0; i < scopeSize; ++i)
    {
        // Media móvil de 5 puntos para suavizar el ruido de cuantización
        float v = scope[i];
        if (i >= 2 && i < scopeSize - 2)
            v = (scope[i - 2] + scope[i - 1] + scope[i] + scope[i + 1] + scope[i + 2]) / 5.0f;

        float x = aX + juce::jmap((float)i, 0.0f, (float)(scopeSize - 1), 0.0f, aW);
        float y = aY + juce::jmap(v, 0.0f, 1.0f, aH, 0.0f);
        pts.add({ x, y });
    }

    curve.startNewSubPath(pts[0]);
    for (int i = 1; i < pts.size(); ++i)
    {
        const auto& p1 = pts[i - 1];
        const auto& p2 = pts[i];
        const auto  mid = juce::Point<float>((p1.x + p2.x) * 0.5f, (p1.y + p2.y) * 0.5f);
        curve.quadraticTo(p1.x, p1.y, mid.x, mid.y);
    }
    curve.lineTo(pts.getLast());

    if (filled)
    {
        // Relleno con gradiente vertical
        juce::Path fill = curve;
        fill.lineTo(area.getBottomRight());
        fill.lineTo(area.getBottomLeft());
        fill.closeSubPath();

        juce::ColourGradient grad(colour.withAlpha(0.2f), aX, aY,
            juce::Colours::transparentBlack, aX, area.getBottom(), false);
        g.setGradientFill(grad);
        g.fillPath(fill);
    }

    // Trazo de la curva (siempre, tanto para filled como para no-filled)
    g.setColour(colour.withAlpha(filled ? 0.85f : 1.0f));
    g.strokePath(curve, juce::PathStrokeType(filled ? 1.5f : 2.0f));
}

void SpectrumAnalyzerAudioProcessorEditor::drawThresholdLine(
    juce::Graphics& g, juce::Rectangle<float> area)
{
    const float threshDb = audioProcessor.apvts.getRawParameterValue("threshold")->load();
    const float threshY = area.getY() +
        juce::jmap(threshDb, 0.0f, -100.0f, 0.0f, area.getHeight());

    // Para líneas discontinuas en JUCE: primero generar el path discontinuo,
    // luego strokePath con solo 2 argumentos (path + tipo de trazo)
    juce::Path sourceLine;
    sourceLine.startNewSubPath(area.getX(), threshY);
    sourceLine.lineTo(area.getRight(), threshY);

    const float dashes[] = { 6.0f, 4.0f };  // 6px trazo, 4px hueco
    juce::Path dashedPath;
    juce::PathStrokeType(1.5f).createDashedStroke(
        dashedPath, sourceLine, dashes, 2);  // 2 = número de elementos en dashes[]

    g.setColour(juce::Colour(0xFFE63946));
    g.strokePath(dashedPath, juce::PathStrokeType(1.5f));  // solo 2 args

    // Etiqueta del valor
    g.setFont(juce::Font(10.0f));
    g.drawText(juce::String(threshDb, 1) + " dB",
        (int)area.getX() + 4, (int)threshY - 13, 60, 11,
        juce::Justification::left);
}

void SpectrumAnalyzerAudioProcessorEditor::drawFrequencyGrid(
    juce::Graphics& g, juce::Rectangle<float> area)
{
    g.setColour(juce::Colour(0xFF1A1E26));
    g.setFont(juce::Font(9.0f));

    // Líneas verticales de frecuencia
    const float freqs[] = { 50, 100, 200, 500, 1000, 2000, 5000, 10000 };
    const double sr = audioProcessor.getSampleRate() > 0 ? audioProcessor.getSampleRate() : 44100.0;

    for (float freq : freqs)
    {
        // Posición X logarítmica: misma fórmula que en el scope
        float norm = std::log10(freq / 20.0f) / std::log10(20000.0f / 20.0f);
        float x = area.getX() + norm * area.getWidth();

        g.drawVerticalLine((int)x, area.getY(), area.getBottom());

        juce::String label = (freq >= 1000) ? juce::String(freq / 1000.0f, 0) + "k"
            : juce::String((int)freq);
        g.setColour(juce::Colour(0xFF3A3F4A));
        g.drawText(label, (int)x - 10, (int)area.getBottom() - 14, 22, 12,
            juce::Justification::centred);
        g.setColour(juce::Colour(0xFF1A1E26));
    }

    // Líneas horizontales de amplitud (dBFS)
    const float dbMarks[] = { -6, -12, -24, -48, -72 };
    for (float db : dbMarks)
    {
        float y = area.getY() + juce::jmap(db, 0.0f, -100.0f, 0.0f, area.getHeight());
        g.setColour(juce::Colour(0xFF1A1E26));
        g.drawHorizontalLine((int)y, area.getX(), area.getRight());
        g.setColour(juce::Colour(0xFF3A3F4A));
        g.drawText(juce::String((int)db), (int)area.getX() + 2, (int)y - 10, 28, 10,
            juce::Justification::left);
    }
}

void SpectrumAnalyzerAudioProcessorEditor::drawVUMeter(
    juce::Graphics& g, juce::Rectangle<int> area)
{
    g.setColour(juce::Colour(0xFF080B0F));
    g.fillRect(area);
    g.setColour(juce::Colour(0xFF1E2128));
    g.drawRect(area, 1);

    // Barra de nivel (canal izquierdo)
    const float fillH = vuLeft * (float)area.getHeight();
    auto fillRect = area.withTop(area.getBottom() - (int)fillH);

    juce::ColourGradient grad(juce::Colour(0xFFE63946), area.getX(), area.getY(),
        juce::Colour(0xFF06D6A0), area.getX(), area.getBottom(), false);
    grad.addColour(0.15, juce::Colour(0xFFFFB703));
    g.setGradientFill(grad);
    g.fillRect(fillRect);

    // Marcas de dB
    g.setFont(juce::Font(8.5f));
    const float dbMarks[] = { 0, -6, -12, -24, -48 };
    for (float db : dbMarks)
    {
        float y = juce::jmap(db, -60.0f, 0.0f, (float)area.getBottom(), (float)area.getY());
        g.setColour(juce::Colour(0xFF3A3F4A));
        g.drawHorizontalLine((int)y, area.getX() - 6, area.getX());
        g.setColour(juce::Colour(0xFF666A73));
        g.drawText(juce::String((int)db), area.getX() - 26, (int)y - 5, 19, 10,
            juce::Justification::right);
    }
}

void SpectrumAnalyzerAudioProcessorEditor::drawGRMeter(
    juce::Graphics& g, juce::Rectangle<int> area)
{
    g.setColour(juce::Colour(0xFF080B0F));
    g.fillRect(area);
    g.setColour(juce::Colour(0xFF1E2128));
    g.drawRect(area, 1);

    // GR en dB: es <= 0. Normalizamos: 0dB = sin reducción (arriba), -20dB = máximo (abajo)
    const float grDb = audioProcessor.getGainReductionDb();
    const float grNorm = juce::jlimit(0.0f, 1.0f, juce::jmap(grDb, 0.0f, -20.0f, 0.0f, 1.0f));
    const int   barH = (int)(grNorm * area.getHeight());

    g.setColour(juce::Colour(0xFFFFB703).withAlpha(0.85f));
    g.fillRect(area.withTop(area.getY()).withHeight(barH));

    // Etiqueta "GR"
    g.setColour(juce::Colour(0xFF666A73));
    g.setFont(juce::Font(9.0f));
    g.drawText("GR", area.getX(), area.getBottom() + 2, area.getWidth(), 12,
        juce::Justification::centred);

    // Valor numérico
    g.setColour(juce::Colour(0xFFFFB703));
    g.drawText(juce::String(grDb, 1), area.getX(), area.getBottom() + 14, area.getWidth(), 12,
        juce::Justification::centred);
}

// =============================================================================
//  resized — Layout de componentes JUCE
// =============================================================================
void SpectrumAnalyzerAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    auto controlsArea = bounds.removeFromBottom(130);
    bounds.removeFromRight(100); // espacio para los meters (solo en el layout de controles)

    // Freeze — esquina superior izquierda del espectro
    freezeButton.setBounds(8, 8, 65, 22);

    // Controles: distribuidos horizontalmente en el área inferior
    controlsArea.reduce(12, 10);

    auto placeSlider = [&](juce::Slider& slider, juce::Label& label, int width)
        {
            auto col = controlsArea.removeFromLeft(width);
            controlsArea.removeFromLeft(8); // separación entre controles
            label.setBounds(col.removeFromTop(18));
            slider.setBounds(col);
        };

    placeSlider(thresholdSlider, thresholdLabel, 100);
    placeSlider(releaseSlider, releaseLabel, 100);
    placeSlider(outputGainSlider, outputGainLabel, 100);
}