These are some sample NAM models designed to be used for performance testing on CPU-limited devices. They are all based on a [LiveSpice model of a Boss SD-1 pedal](https://blog.nostatic.org/2023/04/this-boss-sd-1-pedal-does-not-exist.html).

All models were trained for 300 epochs using the NAM "v1_1_1.wav" capture signal.

| Model | ESR | CPU%<br>(RPi4 64bit) | Notes |
| --- |--- | :-: | --- |
| [BossWN-feather.nam](https://github.com/mikeoliphant/neural-amp-modeler-lv2/blob/main/models/BossWN-feather.nam) | .0001 | 37% | WaveNet "feather" preset |
| [BossWN-4x2x1.nam](https://github.com/mikeoliphant/neural-amp-modeler-lv2/blob/main/models/BossWN-4x2x1.nam) | .0003 | 28% | WaveNet 4x2 channel |
| [BossLSTM-2x16.nam](https://github.com/mikeoliphant/neural-amp-modeler-lv2/blob/main/models/BossLSTM-2x16.nam) | .0013 | 28% | LSTM 2x16 layers |
| [BossLSTM-1x24.nam](https://github.com/mikeoliphant/neural-amp-modeler-lv2/blob/main/models/BossLSTM-1x24.nam) | .0017 | 22% | LSTM 1x24 layer |
| [BossLSTM-2x8.nam](https://github.com/mikeoliphant/neural-amp-modeler-lv2/blob/main/models/BossLSTM-2x8.nam) | .0019 | 17% | LSTM 2x8 layers |
| [BossLSTM-1x16.nam](https://github.com/mikeoliphant/neural-amp-modeler-lv2/blob/main/models/BossLSTM-1x16.nam) | .0041 | 15% | LSTM 1x16 layer |
