# neural-amp-modeler-lv2

Bare-bones implementation of [Neural Amp Modeler](https://github.com/sdatkinson/neural-amp-modeler) (NAM) models in an LV2 plugin.

There is no user interface. Setting the model to use requires that your LV2 host supports atom:Path parameters. Reaper does not. Carla and Ardour do.

To get the intended behavior, you must run your audio host at the same sample rate the model was trained at (usually 48kHz) - no resampling is done by the plugin.

### Compiling

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

