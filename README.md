# ESP-L1 — Brick Wall Limiter

A VST3 and Standalone audio plugin developed in C++17 using the **JUCE Framework**. Implements a time-domain brick-wall limiter with a dual real-time analyzer: scrolling oscilloscope and frequency spectrum.

![ESP-L1](screenshot.png)

## ⚙️ DSP Architecture

The audio processing is built around a standard time-domain envelope follower and a hard-ceiling stage, ensuring absolute peak containment.

* **Detection & Envelope:** Linked-stereo peak detection (`std::max` across channels per sample). Instantaneous attack (0 ms) with exponential release. The release coefficient is calculated from the host sample rate so timing stays consistent regardless of session settings.
* **Gain Reduction:** `GR = thresholdLinear / envelopeState` whenever the envelope exceeds the threshold.
* **Output Gain:** Post-reduction makeup gain applied before the output stage.

## 📊 Visualizers

* **Oscilloscope:** 131 072-sample ring buffer (~3 s at 44.1 kHz). Drawn as min/max bars per pixel for accurate peak representation. PRE (pre-limiter) and POST (post-limiter) signals overlaid. Threshold lines shown as dashed overlays. FREEZE button locks the display.
* **Spectrum Analyzer:** 2048-point Hann-windowed FFT on both pre- and post-limiter signals. Logarithmic frequency axis (20 Hz – 20 kHz). PRE curve filled, POST curve solid line.
* **VU + GR Meters:** Stereo VU meter (L/R) with analogue-style decay; GR meter shows instantaneous gain reduction in dB.

## 🎨 Themes

Three vintage-analog themes, cycled with the THEME button:

| Theme | Style |
|-------|-------|
| AMBER | Warm tube, amber/gold palette |
| PHOSPHOR | Green CRT phosphor monitor |
| STEEL | Dark rack-unit, steel blue |

## 🎛️ Parameters

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| Threshold | -60 → 0 dB | 0 dB | Level above which GR engages |
| Release | 10 → 1000 ms | 100 ms | Exponential decay time (log-scaled knob) |
| Out Gain | -12 → +12 dB | 0 dB | Post-limiter makeup gain |

## 🚀 Build

**Requirements:** JUCE Framework, Visual Studio 2022, Windows.

1. Clone the repo.
2. Open `BasicLimiter.jucer` in Projucer and verify your local JUCE module paths.
3. Export to Visual Studio 2022.
4. Open `Builds/VisualStudio2022/BasicLimiter.sln` and build the `BasicLimiter_VST3` target (Release or Debug).
5. Copy the `.vst3` from the output folder to your DAW's VST3 directory.

> If the `.jucer` file is modified (new files, metadata changes), re-export via Projucer before building.

## 📝 License

Open-source — free to use as a reference for JUCE DSP and lock-free GUI synchronization.
