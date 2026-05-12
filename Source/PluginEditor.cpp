#include "PluginProcessor.h"
#include "PluginEditor.h"

// =============================================================================
//  Theme definitions
// =============================================================================
const Theme SpectrumAnalyzerAudioProcessorEditor::kThemes[kNumThemes] =
{
    // ---- 0: AMBER  (warm vintage tube) ------------------------------------
    {
        "AMBER",
        juce::Colour(0xFF0C0906),   // chassis
        juce::Colour(0xFF1A1208),   // panelBg
        juce::Colour(0xFF201808),   // headerBg
        juce::Colour(0xFF0D0803),   // waveBg
        juce::Colour(0xFF040608),   // specBg
        juce::Colour(0xFF4A3010),   // waveBorder
        juce::Colour(0xFF102018),   // specBorder
        juce::Colour(0xFF4A3418),   // panelBorder
        juce::Colour(0xFFD4A030),   // titleText
        juce::Colour(0xFFB08030),   // labelText
        juce::Colour(0xFF6A4820),   // displayText
        juce::Colour(0xFFE08018),   // waveformPre
        juce::Colour(0xFF68C028),   // waveformPost
        juce::Colour(0xFFBB2010),   // threshold
        juce::Colour(0xFFD4890A),   // grBar
        juce::Colour(0xFF40B840),   // vuGreen
        juce::Colour(0xFF5A4020), juce::Colour(0xFF201408),  // knobRingA/B
        juce::Colour(0xFF2E2214), juce::Colour(0xFF100C06),  // knobBodyA/B
        juce::Colour(0xFFD4890A),   // knobArc
        juce::Colour(0xFFF0D060),   // knobPointer
        juce::Colour(0xFF0A0806),   // tbBg
        juce::Colour(0xFFB08030),   // tbText
    },

    // ---- 1: PHOSPHOR  (green CRT phosphor) --------------------------------
    {
        "PHOSPHOR",
        juce::Colour(0xFF080C08),
        juce::Colour(0xFF101810),
        juce::Colour(0xFF0C180C),
        juce::Colour(0xFF040804),
        juce::Colour(0xFF030603),
        juce::Colour(0xFF1C401C),
        juce::Colour(0xFF183018),
        juce::Colour(0xFF1A3A1A),
        juce::Colour(0xFF70E040),   // titleText  (bright phosphor)
        juce::Colour(0xFF50A830),   // labelText
        juce::Colour(0xFF2A6018),   // displayText
        juce::Colour(0xFF90F050),   // waveformPre
        juce::Colour(0xFF40A820),   // waveformPost
        juce::Colour(0xFFE04020),   // threshold
        juce::Colour(0xFF70E040),   // grBar
        juce::Colour(0xFF60D830),   // vuGreen
        juce::Colour(0xFF284028), juce::Colour(0xFF101808),
        juce::Colour(0xFF1C2C1C), juce::Colour(0xFF0C140C),
        juce::Colour(0xFF70E040),   // knobArc
        juce::Colour(0xFFA0FF60),   // knobPointer
        juce::Colour(0xFF060C06),
        juce::Colour(0xFF50A830),
    },

    // ---- 2: STEEL  (dark modern rack unit) --------------------------------
    {
        "STEEL",
        juce::Colour(0xFF0C0E12),
        juce::Colour(0xFF141828),
        juce::Colour(0xFF101520),
        juce::Colour(0xFF060810),
        juce::Colour(0xFF050608),
        juce::Colour(0xFF203048),
        juce::Colour(0xFF182030),
        juce::Colour(0xFF203048),
        juce::Colour(0xFF80B0E0),   // titleText (steel blue)
        juce::Colour(0xFF6090C0),   // labelText
        juce::Colour(0xFF304870),   // displayText
        juce::Colour(0xFF60A8F0),   // waveformPre (bright blue)
        juce::Colour(0xFF40E0A8),   // waveformPost (teal)
        juce::Colour(0xFFE04040),   // threshold
        juce::Colour(0xFF4090D0),   // grBar
        juce::Colour(0xFF40B880),   // vuGreen
        juce::Colour(0xFF2A3848), juce::Colour(0xFF101828),
        juce::Colour(0xFF1A2438), juce::Colour(0xFF0C1020),
        juce::Colour(0xFF4090D0),   // knobArc
        juce::Colour(0xFF80C8FF),   // knobPointer
        juce::Colour(0xFF060810),
        juce::Colour(0xFF6090C0),
    },
};

// =============================================================================
//  Layout constants
// =============================================================================
static constexpr int kHeaderH  = 32;
static constexpr int kWaveH    = 200;
static constexpr int kGapH     = 10;   // clear gap between displays
static constexpr int kSpecH    = 125;
static constexpr int kControlH = 160;
static constexpr int kMeterW   = 100;
// Total height: 32+200+10+125+160 = 527

// =============================================================================
//  VintageLookAndFeel
// =============================================================================
VintageLookAndFeel::VintageLookAndFeel()
{
    // Defaults — will be overridden by setTheme()
}

void VintageLookAndFeel::setTheme(const Theme& theme)
{
    t = &theme;
    setColour(juce::Label::textColourId,               theme.labelText);
    setColour(juce::Slider::textBoxTextColourId,       theme.tbText);
    setColour(juce::Slider::textBoxBackgroundColourId, theme.tbBg);
    setColour(juce::Slider::textBoxOutlineColourId,    theme.knobBodyB);
    setColour(juce::Slider::textBoxHighlightColourId,  theme.knobArc.withAlpha(0.3f));
}

void VintageLookAndFeel::drawRotarySlider(
    juce::Graphics& g, int x, int y, int width, int height,
    float sliderPos, float startAngle, float endAngle, juce::Slider&)
{
    if (!t) return;

    const float cx     = x + width  * 0.5f;
    const float cy     = y + height * 0.5f;
    const float radius = juce::jmin(width, height) * 0.5f - 6.0f;
    const float angle  = startAngle + sliderPos * (endAngle - startAngle);

    // Outer ring
    {
        const float outerR = radius + 5.0f;
        juce::ColourGradient g1(t->knobRingA, cx - outerR, cy - outerR,
                                t->knobRingB, cx + outerR, cy + outerR, false);
        g.setGradientFill(g1);
        g.fillEllipse(cx - outerR, cy - outerR, outerR * 2.0f, outerR * 2.0f);
        g.setColour(t->knobRingA.brighter(0.15f));
        g.drawEllipse(cx - outerR, cy - outerR, outerR * 2.0f, outerR * 2.0f, 1.0f);
    }

    // Track arc
    {
        juce::Path track;
        track.addArc(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f,
                     startAngle, endAngle, true);
        g.setColour(t->knobBodyB.brighter(0.05f));
        g.strokePath(track, juce::PathStrokeType(5.0f,
            juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Value arc
    {
        juce::Path val;
        val.addArc(cx - radius, cy - radius, radius * 2.0f, radius * 2.0f,
                   startAngle, angle, true);
        g.setColour(t->knobArc);
        g.strokePath(val, juce::PathStrokeType(5.0f,
            juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // Knob body
    {
        const float bodyR = radius - 7.0f;
        juce::ColourGradient g2(t->knobBodyA, cx - bodyR * 0.4f, cy - bodyR * 0.6f,
                                t->knobBodyB, cx + bodyR * 0.4f, cy + bodyR * 0.6f, false);
        g.setGradientFill(g2);
        g.fillEllipse(cx - bodyR, cy - bodyR, bodyR * 2.0f, bodyR * 2.0f);
        g.setColour(t->knobBodyA.withAlpha(0.6f));
        g.drawEllipse(cx - bodyR, cy - bodyR, bodyR * 2.0f, bodyR * 2.0f, 0.8f);

        // Pointer
        const float tipR = bodyR * 0.70f;
        const float tipX = cx + tipR * std::sin(angle);
        const float tipY = cy - tipR * std::cos(angle);
        g.setColour(t->knobPointer);
        g.drawLine(cx, cy, tipX, tipY, 2.0f);
        g.fillEllipse(tipX - 3.0f, tipY - 3.0f, 6.0f, 6.0f);
    }
}

void VintageLookAndFeel::drawButtonBackground(
    juce::Graphics& g, juce::Button& button,
    const juce::Colour&, bool isOver, bool isDown)
{
    const juce::Colour arcCol = t ? t->knobArc  : juce::Colour(0xFFD4890A);
    const juce::Colour dimCol = t ? t->knobBodyA : juce::Colour(0xFF201808);
    const bool toggled = button.getToggleState();

    auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
    juce::Colour base = toggled ? arcCol.withAlpha(0.25f) : dimCol.withAlpha(0.6f);
    if (isDown)       base = base.brighter(0.3f);
    else if (isOver)  base = base.brighter(0.15f);

    g.setColour(base);
    g.fillRoundedRectangle(bounds, 3.0f);
    g.setColour(toggled ? arcCol : dimCol.brighter(0.4f));
    g.drawRoundedRectangle(bounds, 3.0f, 1.5f);

    if (toggled)
    {
        g.setColour(arcCol.brighter(0.4f));
        g.fillEllipse(bounds.getRight() - 11.0f, bounds.getCentreY() - 3.0f, 6.0f, 6.0f);
    }
}

void VintageLookAndFeel::drawButtonText(
    juce::Graphics& g, juce::TextButton& button, bool, bool)
{
    const juce::Colour onCol  = t ? t->knobArc.brighter(0.2f) : juce::Colour(0xFFE0B040);
    const juce::Colour offCol = t ? t->displayText              : juce::Colour(0xFF6A4820);
    g.setFont(juce::Font(10.0f, juce::Font::bold));
    g.setColour(button.getToggleState() ? onCol : offCol);
    g.drawText(button.getButtonText(), button.getLocalBounds(), juce::Justification::centred);
}

// =============================================================================
//  Constructor / Destructor
// =============================================================================
SpectrumAnalyzerAudioProcessorEditor::SpectrumAnalyzerAudioProcessorEditor(
    SpectrumAnalyzerAudioProcessor& p)
    : AudioProcessorEditor(&p),
      audioProcessor(p),
      thresholdAttach (p.apvts, "threshold",  thresholdSlider),
      releaseAttach   (p.apvts, "release",    releaseSlider),
      outputGainAttach(p.apvts, "outputGain", outputGainSlider),
      themeButton(kThemes[0].name)
{
    setLookAndFeel(&vintageLF);
    vintageLF.setTheme(kThemes[0]);

    auto setupSlider = [&](juce::Slider& s, juce::Label& l,
                           const juce::String& name, const juce::String& suffix)
    {
        s.setSliderStyle(juce::Slider::RotaryVerticalDrag);
        s.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 70, 16);
        s.setTextValueSuffix(suffix);
        addAndMakeVisible(s);
        l.setText(name, juce::dontSendNotification);
        l.setJustificationType(juce::Justification::centred);
        l.setFont(juce::Font(10.0f, juce::Font::bold));
        addAndMakeVisible(l);
    };

    setupSlider(thresholdSlider,  thresholdLabel,  "THRESHOLD", " dB");
    setupSlider(releaseSlider,    releaseLabel,    "RELEASE",   " ms");
    setupSlider(outputGainSlider, outputGainLabel, "OUT GAIN",  " dB");

    freezeButton.setClickingTogglesState(true);
    freezeButton.onClick = [this]() { audioProcessor.isFrozen.store(freezeButton.getToggleState()); };
    addAndMakeVisible(freezeButton);

    themeButton.onClick = [this]() { cycleTheme(); };
    addAndMakeVisible(themeButton);

    startTimerHz(30);
    setSize(900, kHeaderH + kWaveH + kGapH + kSpecH + kControlH);
}

SpectrumAnalyzerAudioProcessorEditor::~SpectrumAnalyzerAudioProcessorEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

void SpectrumAnalyzerAudioProcessorEditor::cycleTheme()
{
    themeIndex = (themeIndex + 1) % kNumThemes;
    const Theme& theme = kThemes[themeIndex];
    vintageLF.setTheme(theme);
    themeButton.setButtonText(theme.name);
    repaint();
    thresholdSlider.repaint();
    releaseSlider.repaint();
    outputGainSlider.repaint();
}

// =============================================================================
//  timerCallback
// =============================================================================
void SpectrumAnalyzerAudioProcessorEditor::timerCallback()
{
    const double sr = audioProcessor.getSampleRate();

    const float dbL = juce::Decibels::gainToDecibels(audioProcessor.rmsLeft,  -60.0f);
    const float dbR = juce::Decibels::gainToDecibels(audioProcessor.rmsRight, -60.0f);
    const float tL  = juce::jlimit(0.0f, 1.0f, juce::jmap(dbL, -60.0f, 0.0f, 0.0f, 1.0f));
    const float tR  = juce::jlimit(0.0f, 1.0f, juce::jmap(dbR, -60.0f, 0.0f, 0.0f, 1.0f));
    vuLeft  = tL > vuLeft  ? tL  : juce::jmax(0.0f, vuLeft  - 0.02f);
    vuRight = tR > vuRight ? tR  : juce::jmax(0.0f, vuRight - 0.02f);

    const bool frozen = audioProcessor.isFrozen.load();
    if (!frozen)
        frozenWritePos = audioProcessor.getWaveformWritePos();

    auto processScope = [&](float* fftData, float* scope, auto getReady, auto setReady)
    {
        if (frozen || !getReady()) return;
        audioProcessor.getWindow().multiplyWithWindowingTable(fftData, audioProcessor.fftSize);
        audioProcessor.getFFT().performFrequencyOnlyForwardTransform(fftData);

        for (int i = 0; i < scopeSize; ++i)
        {
            const float freq     = 20.0f * std::pow(1000.0f, (float)i / (float)scopeSize);
            const int   fftIdx   = juce::jlimit(0, audioProcessor.fftSize - 1,
                (int)(freq * audioProcessor.fftSize / sr));
            const float normMag  = fftData[fftIdx] / (audioProcessor.fftSize * 0.5f);
            const float dbFS     = juce::Decibels::gainToDecibels(normMag, -100.0f);
            const float level    = juce::jlimit(0.0f, 1.0f,
                juce::jmap(dbFS, -100.0f, 0.0f, 0.0f, 1.0f));

            if (level >= scope[i]) scope[i] = level;
            else                   scope[i] = juce::jmax(0.0f, scope[i] - 0.035f);
        }
        setReady(false);
    };

    processScope(audioProcessor.getFFTData(), scopeData,
        [this] { return audioProcessor.getNextFFTBlockReady(); },
        [this](bool v) { audioProcessor.setNextFFTBlockReady(v); });
    processScope(audioProcessor.getFFTDataPost(), scopeDataPost,
        [this] { return audioProcessor.getNextFFTBlockReadyPost(); },
        [this](bool v) { audioProcessor.setNextFFTBlockReadyPost(v); });

    repaint();
}

// =============================================================================
//  paint
// =============================================================================
void SpectrumAnalyzerAudioProcessorEditor::paint(juce::Graphics& g)
{
    const Theme& T = kThemes[themeIndex];
    const int W = getWidth();
    const int H = getHeight();
    const int dispW   = W - kMeterW;
    const int waveTop = kHeaderH;
    const int gapTop  = kHeaderH + kWaveH;
    const int specTop = kHeaderH + kWaveH + kGapH;

    // ---- Chassis fill ----
    g.fillAll(T.chassis);

    // ---- Header ----
    {
        juce::ColourGradient hg(T.headerBg, 0, 0, T.chassis, 0, kHeaderH, false);
        g.setGradientFill(hg);
        g.fillRect(0, 0, W, kHeaderH);
        g.setColour(T.panelBorder);
        g.drawHorizontalLine(kHeaderH - 1, 0.0f, (float)W);

        // Plugin name
        g.setFont(juce::Font(17.0f, juce::Font::bold));
        g.setColour(T.titleText);
        g.drawText("ESP-L1", 14, 0, 130, kHeaderH, juce::Justification::centredLeft);

        // Subtitle
        g.setFont(juce::Font(8.5f));
        g.setColour(T.displayText);
        g.drawText("BRICK WALL LIMITER", 104, 0, 130, kHeaderH, juce::Justification::centredLeft);

        // Signal legend — placed after subtitle, well clear of the buttons on the right
        const int lx = 268;
        g.setColour(T.waveformPre);
        g.fillRect(lx, 13, 14, 6);
        g.setFont(juce::Font(8.5f, juce::Font::bold));
        g.setColour(T.displayText);
        g.drawText("PRE", lx + 18, 8, 28, kHeaderH - 16, juce::Justification::centredLeft);
        g.setColour(T.waveformPost);
        g.fillRect(lx + 58, 13, 14, 6);
        g.setColour(T.displayText);
        g.drawText("POST", lx + 76, 8, 32, kHeaderH - 16, juce::Justification::centredLeft);

        // Meter column headers
        g.setFont(juce::Font(8.0f, juce::Font::bold));
        g.setColour(T.displayText);
        const int mx = W - kMeterW + 4;
        g.drawText("L",  mx + 4,  0, 24, kHeaderH, juce::Justification::centred);
        g.drawText("R",  mx + 30, 0, 24, kHeaderH, juce::Justification::centred);
        g.drawText("GR", mx + 62, 0, 30, kHeaderH, juce::Justification::centred);
    }

    // ---- Waveform area ----
    {
        const juce::Rectangle<int> wr(0, waveTop, dispW, kWaveH);
        g.setColour(T.waveBg);
        g.fillRect(wr);
        // Radial warm glow
        juce::ColourGradient vg(T.waveformPre.withAlpha(0.04f),
            (float)wr.getCentreX(), (float)wr.getCentreY(),
            juce::Colours::transparentBlack, (float)wr.getX(), (float)wr.getY(), true);
        g.setGradientFill(vg);
        g.fillRect(wr);
        g.setColour(T.waveBorder);
        g.drawRect(wr, 1);

        // Section label
        g.setFont(juce::Font(9.0f, juce::Font::bold));
        g.setColour(T.waveBorder.brighter(0.5f));
        g.drawText("OSCILLOSCOPE", dispW - 110, waveTop + 5, 105, 13, juce::Justification::right);

        drawWaveform(g, wr.toFloat().reduced(1.0f));
    }

    // ---- Gap strip (hard visual separation) ----
    {
        // Fill gap with chassis color + thin accent lines for structure
        g.setColour(T.chassis);
        g.fillRect(0, gapTop, dispW, kGapH);
        g.setColour(T.waveBorder.withAlpha(0.6f));
        g.drawHorizontalLine(gapTop,             0.0f, (float)dispW);
        g.setColour(T.specBorder.withAlpha(0.6f));
        g.drawHorizontalLine(gapTop + kGapH - 1, 0.0f, (float)dispW);
    }

    // ---- Spectrum area ----
    {
        const juce::Rectangle<int> sr(0, specTop, dispW, kSpecH);
        g.setColour(T.specBg);
        g.fillRect(sr);
        juce::ColourGradient vg2(T.waveformPost.withAlpha(0.04f),
            (float)sr.getCentreX(), (float)sr.getCentreY(),
            juce::Colours::transparentBlack, (float)sr.getX(), (float)sr.getY(), true);
        g.setGradientFill(vg2);
        g.fillRect(sr);
        g.setColour(T.specBorder);
        g.drawRect(sr, 1);

        g.setFont(juce::Font(9.0f, juce::Font::bold));
        g.setColour(T.specBorder.brighter(0.5f));
        g.drawText("SPECTRUM", dispW - 78, specTop + 5, 73, 13, juce::Justification::right);

        auto sf = sr.toFloat().reduced(1.0f);
        drawFrequencyGrid(g, sf);
        drawSpectrumCurve(g, scopeData,     sf, T.waveformPre,  true);
        drawSpectrumCurve(g, scopeDataPost, sf, T.waveformPost, false);
        drawThresholdLine(g, sf);
    }

    // ---- Meters strip ----
    {
        const int meterH = kWaveH + kGapH + kSpecH;
        const juce::Rectangle<int> ms(W - kMeterW, kHeaderH, kMeterW, meterH);
        g.setColour(T.waveBg.darker(0.2f));
        g.fillRect(ms);
        g.setColour(T.specBorder.withAlpha(0.5f));
        g.drawRect(ms, 1);

        const int vuY = ms.getY() + 26;
        const int vuH = ms.getHeight() - 36;
        drawVUMeter(g, { ms.getX() + 6,  vuY, 56, vuH });
        drawGRMeter(g, { ms.getX() + 66, vuY, 26, vuH });
    }

    // ---- Control panel ----
    {
        const juce::Rectangle<int> cp(0, H - kControlH, W, kControlH);
        juce::ColourGradient pg(T.panelBg.brighter(0.1f), 0, (float)(H - kControlH),
                                T.panelBg,                 0, (float)H, false);
        g.setGradientFill(pg);
        g.fillRect(cp);

        g.setColour(T.panelBorder.brighter(0.3f));
        g.drawHorizontalLine(H - kControlH,     0.0f, (float)W);
        g.setColour(T.panelBorder);
        g.drawHorizontalLine(H - kControlH + 1, 0.0f, (float)W);

        // Subtle horizontal texture
        g.setColour(T.panelBg.darker(0.15f).withAlpha(0.5f));
        for (int yy = H - kControlH + 5; yy < H; yy += 4)
            g.drawHorizontalLine(yy, 0.0f, (float)W);
    }
}

// =============================================================================
//  drawWaveform
// =============================================================================
void SpectrumAnalyzerAudioProcessorEditor::drawWaveform(
    juce::Graphics& g, juce::Rectangle<float> area)
{
    const Theme& T    = kThemes[themeIndex];
    const float  aX   = area.getX();
    const float  aR   = area.getRight();
    const float  cy   = area.getCentreY();
    const float  hH   = area.getHeight() * 0.45f;
    const int    dispW = (int)area.getWidth();

    // Amplitude grid
    const float dbGrid[] = { -6.0f, -12.0f, -24.0f };
    for (float db : dbGrid)
    {
        const float amp = juce::Decibels::decibelsToGain(db);
        g.setColour(T.waveBorder.withAlpha(0.5f));
        g.drawHorizontalLine((int)(cy - amp * hH), aX, aR);
        g.drawHorizontalLine((int)(cy + amp * hH), aX, aR);
        g.setFont(juce::Font(7.5f));
        g.setColour(T.displayText);
        g.drawText(juce::String((int)db), (int)aX + 4, (int)(cy - amp * hH) - 10, 36, 9,
                   juce::Justification::left);
    }
    // Zero line
    g.setColour(T.waveBorder.brighter(0.3f));
    g.drawHorizontalLine((int)cy, aX, aR);

    // Waveform buffers
    const int   bufSize = SpectrumAnalyzerAudioProcessor::waveformSize;
    const int   wp      = frozenWritePos;
    const int   spp     = juce::jmax(1, bufSize / dispW);
    const float* pre    = audioProcessor.getWaveformPre();
    const float* post   = audioProcessor.getWaveformPost();

    auto drawBuf = [&](const float* buf, juce::Colour col, float alpha)
    {
        g.setColour(col.withAlpha(alpha));
        for (int px = 0; px < dispW; ++px)
        {
            const int base = wp - (dispW - px) * spp;
            float mn = 0.0f, mx = 0.0f;
            for (int s = 0; s < spp; ++s)
            {
                const float v = buf[((base + s) % bufSize + bufSize) % bufSize];
                if (v < mn) mn = v;
                if (v > mx) mx = v;
            }
            const float y1   = cy - mx * hH;
            const float barH = juce::jmax(1.0f, (cy - mn * hH) - y1);
            g.fillRect(aX + (float)px, y1, 1.0f, barH);
        }
    };

    drawBuf(post, T.waveformPost, 0.55f);
    drawBuf(pre,  T.waveformPre,  0.82f);

    // Threshold lines (symmetric ±)
    const float threshDb  = audioProcessor.apvts.getRawParameterValue("threshold")->load();
    const float threshLin = juce::Decibels::decibelsToGain(threshDb);
    const float tyP = cy - threshLin * hH;
    const float tyN = cy + threshLin * hH;

    const float dashes[] = { 8.0f, 5.0f };
    auto dash = [&](float y) -> juce::Path
    {
        juce::Path s, o;
        s.startNewSubPath(aX, y); s.lineTo(aR, y);
        juce::PathStrokeType(1.5f).createDashedStroke(o, s, dashes, 2);
        return o;
    };
    g.setColour(T.threshold);
    g.strokePath(dash(tyP), juce::PathStrokeType(1.5f));
    g.strokePath(dash(tyN), juce::PathStrokeType(1.5f));

    g.setFont(juce::Font(9.0f, juce::Font::bold));
    g.setColour(T.threshold.withAlpha(0.85f));
    g.drawText(juce::String(threshDb, 1) + " dB",
               (int)aX + 4, (int)tyP - 13, 62, 11, juce::Justification::left);
}

// =============================================================================
//  drawSpectrumCurve
// =============================================================================
void SpectrumAnalyzerAudioProcessorEditor::drawSpectrumCurve(
    juce::Graphics& g, float* scope, juce::Rectangle<float> area,
    juce::Colour colour, bool filled)
{
    const float aX = area.getX(), aY = area.getY();
    const float aW = area.getWidth(), aH = area.getHeight();

    juce::Path curve;
    juce::Array<juce::Point<float>> pts;
    pts.ensureStorageAllocated(scopeSize);

    for (int i = 0; i < scopeSize; ++i)
    {
        float v = scope[i];
        if (i >= 2 && i < scopeSize - 2)
            v = (scope[i-2] + scope[i-1] + scope[i] + scope[i+1] + scope[i+2]) / 5.0f;
        pts.add({ aX + juce::jmap((float)i, 0.0f, (float)(scopeSize-1), 0.0f, aW),
                  aY + juce::jmap(v, 0.0f, 1.0f, aH, 0.0f) });
    }

    curve.startNewSubPath(pts[0]);
    for (int i = 1; i < pts.size(); ++i)
    {
        const auto& p1 = pts[i-1], p2 = pts[i];
        const auto  mid = juce::Point<float>((p1.x+p2.x)*0.5f, (p1.y+p2.y)*0.5f);
        curve.quadraticTo(p1.x, p1.y, mid.x, mid.y);
    }
    curve.lineTo(pts.getLast());

    if (filled)
    {
        juce::Path fill = curve;
        fill.lineTo(area.getBottomRight());
        fill.lineTo(area.getBottomLeft());
        fill.closeSubPath();
        juce::ColourGradient grad(colour.withAlpha(0.22f), aX, aY,
            juce::Colours::transparentBlack, aX, area.getBottom(), false);
        g.setGradientFill(grad);
        g.fillPath(fill);
    }
    g.setColour(colour.withAlpha(filled ? 0.88f : 1.0f));
    g.strokePath(curve, juce::PathStrokeType(filled ? 1.6f : 2.0f));
}

// =============================================================================
//  drawThresholdLine  (spectrum scale: dB mapped to y)
// =============================================================================
void SpectrumAnalyzerAudioProcessorEditor::drawThresholdLine(
    juce::Graphics& g, juce::Rectangle<float> area)
{
    const Theme& T      = kThemes[themeIndex];
    const float thDb    = audioProcessor.apvts.getRawParameterValue("threshold")->load();
    const float y       = area.getY() + juce::jmap(thDb, 0.0f, -100.0f, 0.0f, area.getHeight());

    juce::Path line, dashed;
    line.startNewSubPath(area.getX(), y);
    line.lineTo(area.getRight(), y);
    const float dashes[] = { 7.0f, 5.0f };
    juce::PathStrokeType(1.5f).createDashedStroke(dashed, line, dashes, 2);

    g.setColour(T.threshold);
    g.strokePath(dashed, juce::PathStrokeType(1.5f));
    g.setFont(juce::Font(8.5f, juce::Font::bold));
    g.setColour(T.threshold.withAlpha(0.85f));
    g.drawText(juce::String(thDb, 1) + " dB",
               (int)area.getX() + 4, (int)y - 12, 62, 10, juce::Justification::left);
}

// =============================================================================
//  drawFrequencyGrid
// =============================================================================
void SpectrumAnalyzerAudioProcessorEditor::drawFrequencyGrid(
    juce::Graphics& g, juce::Rectangle<float> area)
{
    const Theme& T = kThemes[themeIndex];

    const float freqs[] = { 50, 100, 200, 500, 1000, 2000, 5000, 10000 };
    for (float freq : freqs)
    {
        const float x = area.getX() +
            (std::log10(freq / 20.0f) / std::log10(1000.0f)) * area.getWidth();
        g.setColour(T.waveBorder.withAlpha(0.4f));
        g.drawVerticalLine((int)x, area.getY(), area.getBottom());
        g.setFont(juce::Font(7.5f));
        g.setColour(T.displayText);
        const juce::String lbl = freq >= 1000 ? juce::String(freq/1000.f,0)+"k"
                                              : juce::String((int)freq);
        g.drawText(lbl, (int)x - 12, (int)area.getBottom() - 13, 26, 11,
                   juce::Justification::centred);
    }

    const float dbMarks[] = { -6, -12, -24, -48, -72 };
    for (float db : dbMarks)
    {
        const float y = area.getY() + juce::jmap(db, 0.0f, -100.0f, 0.0f, area.getHeight());
        g.setColour(T.specBorder.withAlpha(0.5f));
        g.drawHorizontalLine((int)y, area.getX(), area.getRight());
        g.setFont(juce::Font(7.5f));
        g.setColour(T.displayText);
        g.drawText(juce::String((int)db), (int)area.getX()+3, (int)y-9, 28, 9,
                   juce::Justification::left);
    }
}

// =============================================================================
//  drawVUMeter
// =============================================================================
void SpectrumAnalyzerAudioProcessorEditor::drawVUMeter(
    juce::Graphics& g, juce::Rectangle<int> area)
{
    const Theme& T = kThemes[themeIndex];
    const int gap = 4, barW = (area.getWidth() - gap) / 2;

    auto drawBar = [&](juce::Rectangle<int> bar, float level)
    {
        g.setColour(T.waveBg.darker(0.3f));
        g.fillRect(bar);
        g.setColour(T.specBorder.withAlpha(0.5f));
        g.drawRect(bar, 1);

        const int fillH = (int)(level * bar.getHeight());
        if (fillH > 0)
        {
            juce::ColourGradient grad(T.threshold, bar.getX(), bar.getY(),
                T.vuGreen, bar.getX(), bar.getBottom(), false);
            grad.addColour(0.15, T.grBar);
            g.setGradientFill(grad);
            g.fillRect(bar.withTop(bar.getBottom() - fillH));
        }
    };

    drawBar({ area.getX(),              area.getY(), barW, area.getHeight() }, vuLeft);
    drawBar({ area.getX() + barW + gap, area.getY(), barW, area.getHeight() }, vuRight);

    const float dbTicks[] = { 0, -6, -12, -24, -48 };
    g.setFont(juce::Font(7.0f));
    for (float db : dbTicks)
    {
        const float y = juce::jmap(db, -60.f, 0.f, (float)area.getBottom(), (float)area.getY());
        g.setColour(T.specBorder.withAlpha(0.5f));
        g.drawHorizontalLine((int)y, area.getX()-3, area.getRight()+3);
        g.setColour(T.displayText);
        g.drawText(juce::String((int)db), area.getX()-22, (int)y-5, 18, 10, juce::Justification::right);
    }
}

// =============================================================================
//  drawGRMeter
// =============================================================================
void SpectrumAnalyzerAudioProcessorEditor::drawGRMeter(
    juce::Graphics& g, juce::Rectangle<int> area)
{
    const Theme& T = kThemes[themeIndex];

    g.setColour(T.waveBg.darker(0.3f));
    g.fillRect(area);
    g.setColour(T.specBorder.withAlpha(0.5f));
    g.drawRect(area, 1);

    const float grDb   = audioProcessor.getGainReductionDb();
    const float grNorm = juce::jlimit(0.0f, 1.0f, juce::jmap(grDb, 0.0f, -20.0f, 0.0f, 1.0f));
    const int   barH   = (int)(grNorm * area.getHeight());

    if (barH > 0)
    {
        juce::ColourGradient grad(T.grBar.brighter(0.2f), area.getX(), area.getY(),
                                  T.grBar.darker(0.3f),   area.getX(), area.getBottom(), false);
        g.setGradientFill(grad);
        g.fillRect(area.withHeight(barH));
    }

    const float grTicks[] = { 0, -3, -6, -10, -20 };
    for (float db : grTicks)
    {
        const float y = area.getY() + juce::jmap(db, 0.0f, -20.0f, 0.0f, (float)area.getHeight());
        g.setColour(T.specBorder.withAlpha(0.4f));
        g.drawHorizontalLine((int)y, area.getX(), area.getRight());
    }

    g.setFont(juce::Font(8.0f, juce::Font::bold));
    g.setColour(T.grBar);
    g.drawText(juce::String(grDb, 1),
               area.getX(), area.getBottom() + 3, area.getWidth() + 12, 10,
               juce::Justification::left);
}

// =============================================================================
//  resized
// =============================================================================
void SpectrumAnalyzerAudioProcessorEditor::resized()
{
    const int W = getWidth();
    const int H = getHeight();

    freezeButton.setBounds(W - kMeterW - 88, 5, 78, 22);
    themeButton.setBounds(W - kMeterW - 170, 5, 74, 22);

    auto ctrl = getLocalBounds().withTrimmedTop(H - kControlH).reduced(0, 10);

    const int slotW  = 130;
    const int startX = (W - slotW * 3) / 2;

    auto placeKnob = [&](juce::Slider& s, juce::Label& l, int idx)
    {
        const int slotX = startX + idx * slotW;
        l.setBounds(slotX, ctrl.getY(), slotW, 18);
        s.setBounds(slotX, ctrl.getY() + 18, slotW, ctrl.getHeight() - 18);
    };

    placeKnob(thresholdSlider,  thresholdLabel,  0);
    placeKnob(releaseSlider,    releaseLabel,    1);
    placeKnob(outputGainSlider, outputGainLabel, 2);
}
