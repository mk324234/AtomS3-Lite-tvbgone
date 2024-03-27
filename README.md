# AtomS3-Lite-tvbgone

Port of popular agrimpelhuber/esp8266-tvbgone for AtomS3-Lite (maybe also AtomS3, untested)

Send 100+ IR-Codes by Buttonpress, to turn of any TV, Projector etc. 
It is meant to be flashed to an M5Stack AtomS3-Lite (ESP32-S3FN8), using the Arduino Development Platform. The IRremoteESP8266 + Adafruit neopixel library can also be downloaded using the library manager.

## Credits

Based mainly on Ken Shirriff's (and other contributors', credits [here](https://github.com/shirriff/Arduino-TV-B-Gone)) Arduino port of the original TV-B-Gone
Based partly (important part, though) on Mark Szabo's [IRremoteESP8266](https://github.com/markszabo/IRremoteESP8266), also based on Ken Shirriff's work. Thanks, Ken!
And, of course, agrimpelhuber/esp8266-tvbgone based on the original TV-B-Gone by Mitch Altman.


## Changes to the original code

- default Region (without jumper) to Europe
- changed Pin Definitions of Button + IR to AtomS3-Lite (Pin41, Pin4)
- changed internal LED to Neopixel of AtomS3-Lite (Pin35)
- replaced code for internal LED to AtomS3-Lite internal neopixel RGB-LED 



## Hardware

no external hardware required. If you need NA (North American etc) IR-codes, just change the main.h file (as i did).

