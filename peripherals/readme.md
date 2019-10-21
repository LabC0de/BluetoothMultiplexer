# Peripherals

## I2C Lcd

The I2C Lcd library is basically a esp32 rework of the arduino library and works without any objects, you just call the functions. So far it only supports one 16x2 lcd, I just dont need the dynamic functionality (should i ever need it ill probably revise this one). See i2cLcd.h for reference.

## Rotary Encoder

Likewise the rotary.h library supports only one rotary encoder (same reason as the Lcd). You just initialize the encoder and use the variables in the header file (will be replaced by same named macros should i ever need more than one rotary enoder). The button variable is true while the integrated button is pressed but this library should also work with non button rotary encoders. See rotary.h for reference.