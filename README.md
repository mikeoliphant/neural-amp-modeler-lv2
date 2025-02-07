# neural-amp-modeler-lv2

LV2 plugin for using neural network machine learning amp models.

**There is no custom plugin user interface**. Setting the model to use requires that your LV2 host supports atom:Path parameters. Reaper does as of v6.82. Carla and Ardour do. If your favorite LV2 host does not support atom:Path, let them know you want it.
If you are looking for a GUI version, @brummer10 [has one here](https://github.com/brummer10/neural-amp-modeler-ui) that works for Linux and Windows. You may also be interested in the the version shipped with the [MOD Desktop App](https://github.com/moddevices/mod-desktop-app), or my digital pedalboard app [Stompbox](https://github.com/mikeoliphant/StompboxUI).

To get the intended behavior, **you must run your audio host at the same sample rate the model was trained at** (usually 48kHz) - no resampling is done by the plugin.

For amp-only models (the most typical), **you will need to run an impulse reponse after this plugin** to model the cabinet.

## Models and Performance

The plugin supports both [Neural Amp Modeler (NAM)](https://github.com/sdatkinson/neural-amp-modeler) models and [RTNeural keras json models](https://github.com/jatinchowdhury18/RTNeural) (like those used by [Aida-X](https://github.com/AidaDSP/AIDA-X)).

The best source of models is [ToneHunt](https://tonehunt.org/).

NAM WaveNet models are generally quite expensive to run. This isn't (much of) an issue on modern PCs, but you may have trouble running on less powerful hardware.

A Raspberry Pi 4 running a 64bit OS can run "standard" NAM models with a bit of room to spare for a cabinet IR and some lightweight effects.

If you are having trouble running a "standard" model, try looking for "feather", or even "nano" (the least expensive) models. You can find a list of ["feather"-tagged models on ToneHunt](https://tonehunt.org/models?tags%5B0%5D=feather-mdl). Note that tagging models is up to the submitter, so not all "feather" models are tagged as such - you should be able to find more if you dig around.

For more information on model type support, see the [NeuralAudio](https://github.com/mikeoliphant/NeuralAudio) repository, which is where the model handling code lives.

## Input Calibration

The expected input level to the plugin is 12dBu. For models that include input level information, they will be calibrated against this level. If you know the input level of your audio interface, you should adjust the input level relative to the expected 12dBu to provide the appropriate signal level to the model.

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

## Optimization

If you have a relatively modern x64 processor, you can pass ```-DUSE_NATIVE_ARCH=ON``` on your cmake command line to enable certain processor-specific optimizations.
