# TFT Touch Screen interface for working with LSM6DS3
Uses [ILI9341 library from maudeve-it](https://github.com/maudeve-it/ILI9XXX-XPT2046-STM32) and a custom HAL-based LSM6DS3 library.

## Features

- **Touch UI** on a 2.4" 320×240 TFT (ILI9341 + XPT2046) with menu: Restart, Plot, I2C Scan, Debug
- **Sensor setup from the screen**: ODR 104–3330 Hz, full-scale ±2/±4/±8/±16 g
- **Live plot** of acceleration vs. time with live X/Y/Z readout in g
- **Axis switching** (X/Y/Z) with the board's user button
- **I2C Scan**: finds the sensor on the bus and checks `WHO_AM_I` (0x69)
- **Debug screen**: shows `CTRL1_XL` and the decoded ODR and range
- **Custom LSM6DS3 driver** (STM32 HAL, I²C) and event-driven touch handling
- **Hardware**: STM32F411 Discovery, LSM6DS3, LOLIN TFT 2.4 Touch Shield

## Connection diagram used in the project:
<img width="1257" height="890" alt="polaczenia" src="https://github.com/user-attachments/assets/f23f4f20-e01b-448b-b8a8-0b4be3939565" />

## Demo:
![LSM6DS3 TFT demo](/lsm6ds3-tft-demo.gif)

