# Basic Limiter

A VST3 and Standalone audio plugin developed in C++17 using the **JUCE Framework**. This project implements a time-domain brickwall limiter with a lock-free, real-time oscilloscope visualizer.

<img width="1439" height="828" alt="image" src="https://github.com/user-attachments/assets/be4884c6-91d7-43df-8d7e-588da3cecdcb" />

## ⚙️ DSP Architecture & Implementation

The audio processing is built around a standard time-domain envelope follower and a hard-clipping stage, ensuring absolute peak containment.

* **Detection & Envelope:** * Uses a Linked-Stereo peak detection algorithm (`std::max` across channels per sample).
  * The envelope features an instantaneous attack (0 ms) and an exponential release. The release coefficient is dynamically calculated based on the host's current sample rate to ensure consistent timing regardless of session settings.
* **Gain Reduction:** * Gain reduction is calculated inversely to the envelope once it exceeds the linear threshold (`thresholdLinear / envelopeState`).
* **Brickwall Ceiling:** * After the gain reduction and output gain are applied, the signal passes through a hard-clipping stage using `juce::jlimit`. The absolute maximum output is clamped to the linear equivalent of the `Ceiling` parameter, ensuring the signal never exceeds this mathematical boundary.

## 📊 Visualizer & Thread Synchronization

To transfer high-frequency audio data to the 60Hz GUI without blocking the audio thread or allocating memory, the plugin uses a lock-free downsampling approach:

* **Downsampling:** The audio thread calculates how many audio samples correspond to a single UI pixel (`sampleRate / 100.0`). It accumulates the maximum input peak and minimum gain reduction within that window.
* **Circular Buffer:** Once a "pixel window" is complete, the data is pushed into fixed-size `std::array` buffers (`inputHistory` and `grHistory`). An `std::atomic<int>` index is incremented and wrapped around, ensuring thread-safe reads from the GUI timer callback.
* **Rendering:** The UI reads backwards from the current atomic write index to draw the historical waveform. The waveform is mirrored symmetrically on the Y-axis for standard oscilloscope aesthetics. VU meters feature custom decay logic to simulate analogue inertia.

## 🎛️ Parameters

* **Threshold (dB):** The level at which the gain reduction envelope engages.
* **Release (ms):** The exponential decay time for the gain reduction envelope.
* **Output Gain (dB):** Linear makeup gain applied post-reduction, pre-ceiling.
* **Ceiling (dB):** The absolute maximum output peak limit (Hard-Clipper).
* **Freeze (Toggle):** Stops the atomic write-index progression, freezing the visualizer state for inspection.

## 🚀 Build Instructions

This repository is configured for **Windows** and **Visual Studio 2022**. Compiled binaries (`.vst3`, `.exe`, etc.) are explicitly ignored via `.gitignore`.

### Prerequisites:
1.  [JUCE Framework](https://juce.com/) installed locally.
2.  **Visual Studio 2022** with "Desktop development with C++" workload installed.

### Steps:
1.  Clone this repository:
    ```bash
    git clone [https://github.com/lluisestape-upc/BasicLimiter.git](https://github.com/lluisestape-upc/BasicLimiter.git)
    ```
2.  Open the `BasicLimiter.jucer` file using the **Projucer** application.
3.  Ensure your JUCE module paths are correctly configured for your local machine within the Projucer.
4.  Click the **Visual Studio 2022** icon in the Projucer to generate the `.sln` and `.vcxproj` files and open the IDE.
5.  Set your build configuration to **Release** (for optimized DSP performance) or **Debug** (for development).
6.  Build the solution (`Ctrl + Shift + B`).
7.  The output files will be located in `Builds/VisualStudio2022/x64/Release/VST3/`.

## 📝 License

This project is open-source. Feel free to use it as a reference for JUCE DSP development and lock-free GUI synchronization.
