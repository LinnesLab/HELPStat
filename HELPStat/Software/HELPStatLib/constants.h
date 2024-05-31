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

// // ESP32 S3 SPI / RESET / INT PINS
#define MOSI 35
#define MISO 37 
#define SCK 36
#define CS 11
#define RESET 10
#define ESP32_INTERRUPT 9

// SD CARD PINS
#define CS_SD 21
// Need to make sure CLK doesn't exceed SD card maximum
// Using the same clock is fine for this case
#define SD_CLK 240000000 / 16
#define FILENAME "EISdata.csv"

/* USING SCL/SDA PINS AS LED DRIVERS FOR OUTPUT */
#define LED1 6
#define LED2 7


#define SERIAL_BAUD 115200 

#endif // CONSTANTS_H