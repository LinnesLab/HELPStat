#ifndef CONSTANTS_H
#define CONSTANTS_H

#define CLCK 240000000 / 16
#define BITS MSBFIRST
#define SPIMODE SPI_MODE0

/* ESP32 Feather */
// SPI / RESET / INT PINS
// #define MOSI 18 
// #define MISO 19 
// #define SCK 5
// #define CS 33
// #define RESET 21
// #define ESP32_INTERRUPT 14 // INTERRUPT PIN FOR ESP32

// Adafruit ESP32-S3 Feather (#5477 / #5323) - SPI / RESET / INT PINS
// SCK/MOSI/MISO below are the Feather's default hardware SPI pins.
#define MOSI 35
#define MISO 37
#define SCK 36
#define CS 11               // D11
#define RESET 10            // D10
#define ESP32_INTERRUPT 9   // D9

// SD CARD PINS
// NOTE: on the Feather, GPIO21 is the onboard NeoPixel power pin. It is reused
// here as the SD chip-select; fine as long as the onboard NeoPixel is unused.
#define CS_SD 21
// Need to make sure CLK doesn't exceed SD card maximum
// Using the same clock is fine for this case
#define SD_CLK 240000000 / 16
#define FILENAME "EISdata.csv"

/* Optional button/LED drivers. NOTE: on the Feather, GPIO7 is the STEMMA-QT
   I2C power pin; reused here, fine as long as STEMMA-QT I2C is unused. */
#define LED1 6
#define LED2 7


#define SERIAL_BAUD 115200 

#endif // CONSTANTS_H