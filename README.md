# neural-amp-modeler-lv2

LV2 plugin for neural network machine learning amp model playback using the [NeuralAudio](https://github.com/mikeoliphant/NeuralAudio) engine.

**There is no custom plugin user interface**. Setting the model to use requires that your LV2 host supports atom:Path parameters. Reaper does as of v6.82. Carla and Ardour do. If your favorite LV2 host does not support atom:Path, let them know you want it.
If you are looking for a GUI version, @brummer10 [has one here](https://github.com/brummer10/neural-amp-modeler-ui) that works for Linux and Windows. You may also be interested in the the version shipped with the [MOD Desktop App](https://github.com/moddevices/mod-desktop-app), or my digital pedalboard app [Stompbox](https://github.com/mikeoliphant/Stompbox).

To get the intended behavior, **you must run your audio host at the same sample rate the model was trained at** (usually 48kHz) - no resampling is done by the plugin. An exception to this is oversampling (running at an even multiple of the model sample rate). See the "Oversampling" section below.

For amp-only models (the most typical), **you will need to run an impulse reponse after this plugin** to model the cabinet.

## Usage

Your DAW should expose the following input controls:

**Input:** - Input (pre-model) gain in dB.

**Output:** - Output (post-model) volume in dB.

**Quality:** - Model quality (if applicable). For NAM A2 models, a value below 0.5 will give you a "lite" model and a value above 0.5 will give you a "full" model.

**Model:** - The model file (ie: xxx.nam) to use.

## Models Supported and Performance

The plugin supports both [Neural Amp Modeler (NAM)](https://github.com/sdatkinson/neural-amp-modeler) models (both A1 and A2) and [RTNeural keras json models](https://github.com/jatinchowdhury18/RTNeural) (like those used by [Aida-X](https://github.com/AidaDSP/AIDA-X)).

The best source of models is [Tone3000](https://www.tone3000.com/).

For more information on model type support and performance, see the [NeuralAudio](https://github.com/mikeoliphant/NeuralAudio) repository, which is where the model handling code lives.

## Input Calibration

The expected input level to the plugin is 12dBu. For models that include input level information, they will be calibrated against this level. If you know the input level of your audio interface, you should adjust the input level relative to the expected 12dBu to provide the appropriate signal level to the model.

## Oversampling

If you run at a sample rate that is an even multiple of the model sample rate, the model will be correctly oversampled to behave as expected. So, if the model sample rate is 48kHz (which they typically are), and you are running at 96kHz, the model will be 2x oversampled. This also means that if your DAW supports oversampling, you can safely use it. Running oversampled comes with a signficant performance cost, but can be useful for reducing aliasing.

## Building

First clone the repository:
```bash
git clone --recurse-submodules -j4 https://github.com/mikeoliphant/neural-amp-modeler-lv2
cd neural-amp-modeler-lv2/build
```

Then compile the plugin using:

**Linux/MacOS**
```bash
cmake .. -DCMAKE_BUILD_TYPE="Release"
make -j4
```

**Windows**
```bash
cmake.exe -G "Visual Studio 17 2022" -A x64 ..
cmake --build . --config=release -j4
```

Note - you'll have to change the Visual Studio version if you are using a different one.

After building, the plugin will be in **build/neural_amp_modeler.lv2**.

## CMake Options

```-DUSE_NATIVE_ARCH=ON```: If you have a relatively modern x64 processor, you can pass ```-DUSE_NATIVE_ARCH=ON``` on your cmake command line to enable certain processor-specific optimizations.

```-DSMART_BYPASS_ENABLED=ON```: If enabled, this will bypass model processing if input has been silent (below -100 dB by default) for a sufficient number of samples (determined by the model's receptive field size).

Also see the [NeuralAudio CMake options](https://github.com/mikeoliphant/NeuralAudio#cmake-options) - adding these to your neural-amp-modeler-lv2 cmake will pass them to the NeuralAudio build.
