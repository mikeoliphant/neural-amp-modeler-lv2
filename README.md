# neural-amp-modeler-lv2

Bare-bones implementation of [Neural Amp Modeler](https://github.com/sdatkinson/neural-amp-modeler) (NAM) models in an LV2 plugin.

There is no user interface. Currently the model path is hardcoded in the plugin code [here](https://github.com/mikeoliphant/neural-amp-modeler-lv2/blob/main/src/nam_plugin.cpp). Yes, this is suboptimal, but it suffices for testing for now. I haven't yet found a way in LV2 to allow the user to select
a file without doing a custom UI (which I don't really want to do).

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

