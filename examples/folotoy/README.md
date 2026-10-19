# FoloToy AI Passport

Runs the shared tinymenu demo on the ESP32-C3 AI Passport with 8 MB flash and
the ST7789P3 240x320 display. The 120x128 UI is scaled 2x into a centred
240x256 viewport, leaving 32 pixels above and below to clear the rounded corners.
All demo readings are simulated.

```sh
pio run -d examples/folotoy
pio run -d examples/folotoy -t upload --upload-port <port>
pio device monitor -d examples/folotoy --port <port>
```

UP/DOWN select the previous/next item and repeat when held. OK confirms;
holding OK goes back. All three keys share GPIO0's ADC ladder. The native USB
serial port also accepts the controls documented in the main README.

Display pins are MOSI 9, SCLK 8, CS 1, DC 20 and backlight 21; reset is software
only. See [src/board.h](src/board.h). The display uses SPI mode 0 at 40 MHz,
RGB colour order and native portrait orientation.

The standalone Arduino partition layout reserves the device's `cardid` region
at `0x356000`; uploading installs this demo in place of the existing firmware.

Panel initialization and ADC thresholds are adapted from FoloToy's
[AI Passport Arduino sources](https://github.com/FoloToy/ai-passport-arduino/tree/b3482d9d813c626c2e6e8d6bf9e267accd28a998),
under the [MIT license](LICENSES/FoloToy-MIT.txt).
