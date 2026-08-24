# LSM303_to_MIDIBLE
accelerometer sensor based wireless MIDI controller


-- -- -- -- --


assembly:

establish I2C communication between a Adafruit ESP32 S3 Feather board and a Adafruit LSM303AGR module with a Qwiic cable and power the Feather biard board with a lipo battery

Feather board: https://www.adafruit.com/product/5323
LSM303ARG module: https://www.adafruit.com/product/4413


-- -- -- -- --


all the code for this project was made using the Arduino IDE and the following libraries:

NimBLE: https://github.com/h2zero/NimBLE-Arduino
Arduino ESP32: https://github.com/espressif/arduino-esp32
Adafruit LSM303 accel: https://github.com/adafruit/Adafruit_LSM303_Accel
