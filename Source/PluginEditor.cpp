#include "PluginProcessor.h"
#include "PluginEditor.h"

SpectrumAnalyzerAudioProcessorEditor::SpectrumAnalyzerAudioProcessorEditor(SpectrumAnalyzerAudioProcessor& p)
    : AudioProcessorEditor(&p), audioProcessor(p),
    thresholdAttach(p.apvts, "threshold", thresholdSlider),
    releaseAttach(p.apvts, "release", releaseSlider),
    outputGainAttach(p.apvts, "outputGain", outputGainSlider),
    ceilingAttach(p.apvts, "ceiling", ceilingSlider)
{
    // Helper para configurar sliders (integrando unidades en el textBox)
    auto setupSlider = [this](juce::Slider& slider, juce::Label& label, const juce::String& text, juce::Colour color, const juce::String& suffix) {
        slider.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        slider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 75, 18);
        slider.setColour(juce::Slider::rotarySliderFillColourId, color);
        slider.setTextValueSuffix(suffix); // UNIDAD AQUÍ
        addAndMakeVisible(slider);

        label.setText(text, juce::dontSendNotification);
        label.setJustificationType(juce::Justification::centred);
        label.setFont(juce::Font(12.0f));
        addAndMakeVisible(label);
        };

    setupSlider(thresholdSlider, thresholdLabel, "Threshold", juce::Colour(0xFF9D4EDD), " dB");
    setupSlider(releaseSlider, releaseLabel, "Release", juce::Colour(0xFF48CAE4), " ms");
    setupSlider(outputGainSlider, outputGainLabel, "Out Gain", juce::Colour(0xFF06D6A0), " dB");
    setupSlider(ceilingSlider, ceilingLabel, "Ceiling", juce::Colour(0xFFEF476F), " dB");

    // Freeze Button reubicado sistemáticamente en resized()
    freezeButton.setClickingTogglesState(true);
    freezeButton.onClick = [this]() { audioProcessor.isFrozen.store(freezeButton.getToggleState()); };
    addAndMakeVisible(freezeButton);

    startTimerHz(60); // 60 FPS da la suavidad del osciloscopio profesional
    setSize(950, 520); // Ligeramente más grande para acomodar los 4 knobs
}

SpectrumAnalyzerAudioProcessorEditor::~SpectrumAnalyzerAudioProcessorEditor()
{
    stopTimer();
}

void SpectrumAnalyzerAudioProcessorEditor::timerCallback()
{
    // Inercia de VU Meters (analógico)
    const float dbL = juce::Decibels::gainToDecibels(audioProcessor.rmsLeft, -60.0f);
    const float dbR = juce::Decibels::gainToDecibels(audioProcessor.rmsRight, -60.0f);
    const float tL = juce::jlimit(0.0f, 1.0f, juce::jmap(dbL, -60.0f, 0.0f, 0.0f, 1.0f));
    const float tR = juce::jlimit(0.0f, 1.0f, juce::jmap(dbR, -60.0f, 0.0f, 0.0f, 1.0f));
    vuLeft = (tL > vuLeft) ? tL : juce::jmax(0.0f, vuLeft - 0.02f);
    vuRight = (tR > vuRight) ? tR : juce::jmax(0.0f, vuRight - 0.02f);

    repaint();
}

void SpectrumAnalyzerAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xFF1E2124)); // Fondo oscuro grisáceo

    auto bounds = getLocalBounds();
    auto controlsArea = bounds.removeFromBottom(135);
    auto metersArea = bounds.removeFromRight(100);
    auto visualizerArea = bounds.toFloat();

    // 1. Fondo del visualizador
    g.setColour(juce::Colour(0xFF121417));
    g.fillRect(visualizerArea);

    // Línea central 0 dB (como referencia central del osciloscopio)
    g.setColour(juce::Colour(0xFF3A3F44));
    g.drawHorizontalLine(static_cast<int>(visualizerArea.getCentreY()),
        visualizerArea.getX(), visualizerArea.getRight());

    // 2. Dibujar Grid de visualización con UNIDADES
    drawTimeGrid(g, visualizerArea);

    // 3. Dibujar Forma de Onda y Envolvente (Osciloscopio)
    drawTimeWaveform(g, visualizerArea);

    // 4. Medidores Pre-limiter y GR
    auto vuArea = metersArea.removeFromLeft(60).reduced(6, 35);
    auto grArea = metersArea.reduced(4, 35);
    drawVUMeter(g, vuArea);
    drawGRMeter(g, grArea);

    // 5. Fondo del área de controles
    g.setColour(juce::Colour(0xFF282B30));
    g.fillRect(controlsArea);
    g.setColour(juce::Colour(0xFF3A3F44));
    g.drawLine((float)controlsArea.getX(), (float)controlsArea.getY(),
        (float)controlsArea.getRight(), (float)controlsArea.getY(), 1.0f);
}

void SpectrumAnalyzerAudioProcessorEditor::drawTimeGrid(juce::Graphics& g, juce::Rectangle<float> area)
{
    g.setColour(juce::Colour(0xFF2A2E33).withAlpha(0.6f));
    g.setFont(juce::Font(10.0f));

    float midY = area.getCentreY();
    float heightMod = area.getHeight() * 0.5f;

    // Líneas de amplitud horizontales
    const float marks[] = { -6, -12, -18, -24 };
    for (float db : marks)
    {
        float threshLinear = juce::Decibels::decibelsToGain(db);
        float threshY = midY - (threshLinear * heightMod);
        float threshY_bottom = midY + (threshLinear * heightMod);

        g.drawHorizontalLine(static_cast<int>(threshY), area.getX(), area.getRight());
        g.drawHorizontalLine(static_cast<int>(threshY_bottom), area.getX(), area.getRight());

        // Etiqueta (con unidades dB)
        if (area.getWidth() > 300) {
            g.drawText(juce::String((int)db) + " dB", area.getX() + 2, static_cast<int>(threshY), 40, 10, juce::Justification::left);
        }
    }
}

void SpectrumAnalyzerAudioProcessorEditor::drawTimeWaveform(juce::Graphics& g, juce::Rectangle<float> area)
{
    int writePos = audioProcessor.historyWriteIndex.load(std::memory_order_acquire);
    const int bufferSize = SpectrumAnalyzerAudioProcessor::visualizerBufferSize;

    juce::Path inputPath;
    juce::Path outputPath;

    float midY = area.getCentreY();
    float heightMod = area.getHeight() * 0.5f;
    float startX = area.getRight();

    inputPath.startNewSubPath(startX, midY);
    outputPath.startNewSubPath(startX, midY);

    // Recorremos los píxeles de derecha (presente) a izquierda (pasado)
    for (int x = 0; x < area.getWidth(); ++x)
    {
        int readPos = writePos - x - 1;
        while (readPos < 0) readPos += bufferSize;

        float inputPeak = audioProcessor.inputHistory[readPos];
        float grMult = audioProcessor.grHistory[readPos];
        float outputPeak = inputPeak * grMult;

        float drawX = startX - x;
        float inputDrawY = midY - (inputPeak * heightMod);
        float outputDrawY = midY - (outputPeak * heightMod);

        inputPath.lineTo(drawX, inputDrawY);
        outputPath.lineTo(drawX, outputDrawY);
    }

    // Espejamos hacia abajo para simular forma de onda simétrica
    for (int x = static_cast<int>(area.getWidth()) - 1; x >= 0; --x)
    {
        int readPos = writePos - x - 1;
        while (readPos < 0) readPos += bufferSize;

        float inputPeak = audioProcessor.inputHistory[readPos];
        float grMult = audioProcessor.grHistory[readPos];
        float outputPeak = inputPeak * grMult;

        float drawX = startX - x;
        float inputDrawY = midY + (inputPeak * heightMod);
        float outputDrawY = midY + (outputPeak * heightMod);

        inputPath.lineTo(drawX, inputDrawY);
        outputPath.lineTo(drawX, outputDrawY);
    }

    inputPath.closeSubPath();
    outputPath.closeSubPath();

    // Dibujar señal de entrada (sombra gris)
    g.setColour(juce::Colours::white.withAlpha(0.2f));
    g.fillPath(inputPath);

    // Dibujar señal limitada (color morado)
    g.setColour(juce::Colour(0xFF9D4EDD).withAlpha(0.8f));
    g.fillPath(outputPath);
    g.setColour(juce::Colour(0xFFC77DFF));
    g.strokePath(outputPath, juce::PathStrokeType(1.0f));

    // --- Línea del Ceiling (ROJO) ---
    const float ceilingDb = audioProcessor.apvts.getRawParameterValue("ceiling")->load();
    const float ceilingLinear = juce::Decibels::decibelsToGain(ceilingDb);
    float ceilY_top = midY - (ceilingLinear * heightMod);
    float ceilY_bottom = midY + (ceilingLinear * heightMod);

    g.setColour(juce::Colour(0xFFEF476F).withAlpha(0.8f)); // Rosa/Rojo
    g.drawHorizontalLine(static_cast<int>(ceilY_top), area.getX(), area.getRight());
    g.drawHorizontalLine(static_cast<int>(ceilY_bottom), area.getX(), area.getRight());
}

void SpectrumAnalyzerAudioProcessorEditor::drawVUMeter(juce::Graphics& g, juce::Rectangle<int> area)
{
    g.setColour(juce::Colour(0xFF121417));
    g.fillRect(area);
    g.setColour(juce::Colour(0xFF3A3F44));
    g.drawRect(area, 1);

    const float fillH = vuLeft * (float)area.getHeight();
    auto fillRect = area.withTop(area.getBottom() - (int)fillH);

    juce::ColourGradient grad(juce::Colour(0xFFE63946), area.getX(), area.getY(),
        juce::Colour(0xFF06D6A0), area.getX(), area.getBottom(), false);
    grad.addColour(0.15, juce::Colour(0xFFFFB703));
    g.setGradientFill(grad);
    g.fillRect(fillRect);
}

void SpectrumAnalyzerAudioProcessorEditor::drawGRMeter(juce::Graphics& g, juce::Rectangle<int> area)
{
    g.setColour(juce::Colour(0xFF121417));
    g.fillRect(area);
    g.setColour(juce::Colour(0xFF3A3F44));
    g.drawRect(area, 1);

    const float grDb = audioProcessor.getGainReductionDb();
    const float grNorm = juce::jlimit(0.0f, 1.0f, juce::jmap(grDb, 0.0f, -20.0f, 0.0f, 1.0f));
    const int   barH = (int)(grNorm * area.getHeight());

    g.setColour(juce::Colour(0xFF9D4EDD).withAlpha(0.9f));
    g.fillRect(area.withTop(area.getY()).withHeight(barH));

    // Etiqueta GR y Valor numérico (con unidades dB)
    g.setColour(juce::Colour(0xFF99AAB5));
    g.setFont(juce::Font(10.0f));
    g.drawText("GR", area.getX(), area.getBottom() + 2, area.getWidth(), 12, juce::Justification::centred);

    g.setColour(juce::Colour(0xFFC77DFF));
    g.drawText(juce::String(grDb, 1) + " dB", area.getX(), area.getBottom() + 14, area.getWidth(), 12, juce::Justification::centred);
}

void SpectrumAnalyzerAudioProcessorEditor::resized()
{
    auto bounds = getLocalBounds();
    auto controlsArea = bounds.removeFromBottom(135); // Área inferior
    bounds.removeFromRight(100);

    // --- REUBICACIÓN DEL FREEZE: ABAJO DERECHA ---
    auto controlsSubBounds = controlsArea;
    int knobWidth = 105;
    auto freezeBounds = controlsSubBounds.removeFromRight(knobWidth / 1.5).reduced(8, 50); // Mismo espacio que un knob
    freezeButton.setBounds(freezeBounds.withHeight(25)); // Un poco más alto que antes

    // --- LAYOUT DE CONTROLES (DISTRIBUCIÓN HORIZONTAL) ---
    controlsArea.reduce(12, 12);
    controlsArea.removeFromRight(knobWidth / 1.5); // Espacio libre para el freeze

    auto placeSlider = [&](juce::Slider& slider, juce::Label& label, int width)
        {
            auto col = controlsArea.removeFromLeft(width);
            controlsArea.removeFromLeft(12); // separación entre controles
            label.setBounds(col.removeFromTop(18));
            slider.setBounds(col);
        };

    placeSlider(thresholdSlider, thresholdLabel, knobWidth);
    placeSlider(releaseSlider, releaseLabel, knobWidth);
    placeSlider(outputGainSlider, outputGainLabel, knobWidth);
    placeSlider(ceilingSlider, ceilingLabel, knobWidth);
}