// Library extension for AD5940.h

/*
I think this was because putting them in a class / object-oriented
structure didn't allow the C file to use them. Had to separate the 
functions for Arduino and functions solely used by the library. 
*/

/*
Updates Section
09/02/2023: Got Initialization working. May need to add MCU flags for ESP32
based on FreiStat and Analog Device documentations / recommendations. 

09/07/2023: Changed code to include constants from AD5940ino.h to have a singular place 
to make edits.
*/

#include "Arduino.h"
#include "SPI.h"
#include <constants.h>

extern "C" { 
#include <ad5940.h> 
}


volatile uint32_t uCInterrupt = 0;
void IRAM_ATTR interruptISR();

void AD5940_RstClr()
{
    digitalWrite(RESET, LOW); 
}

void AD5940_RstSet()
{
    digitalWrite(RESET, HIGH); 
}

void AD5940_CsClr()
{
    digitalWrite(CS, LOW); 
}

void AD5940_CsSet()
{
    digitalWrite(CS, HIGH); 
}

// from FreiStat - not sure if necessary but
// just in case, we'll use it
void AD5940_Delay10us(uint32_t iTime) 
{
    // Value is smaller threshold
    if (iTime < 1638){
        delayMicroseconds(iTime * 10);
    }
    // Value is larger than threshold
    else if (iTime >= 1638){
        uint32_t iTimeDelayMicro = iTime % 1000;
        uint32_t iTimeDelay = iTime - iTimeDelayMicro;
        delay(iTimeDelay / 100);
        delayMicroseconds(iTimeDelayMicro * 10);
    }
}

// writing to SPI 
// Putting the begin / end transactions here were from FreiStat

void AD5940_ReadWriteNBytes(unsigned char *pSendBuffer, unsigned char *pRecvBuff, unsigned long numBytes)
{
    // SPI.beginTransaction(SPISettings(240000000 / 16, MSBFIRST, SPI_MODE0));
    SPI.beginTransaction(SPISettings(CLCK, BITS, SPIMODE));
    for(unsigned long i=0; i < numBytes; i++) { 
        // Sends and receives bytes as specified by numBytes
        pRecvBuff[i] = SPI.transfer(pSendBuffer[i]);
    }
    SPI.endTransaction();
}

/* IINTERRUPT FUNCTIONS */
void IRAM_ATTR interruptISR() {
    uCInterrupt = 1;
}

uint32_t AD5940_ClrMCUIntFlag() {
    uCInterrupt = 0;
    return 0;
}

uint32_t AD5940_GetMCUIntFlag() {
    return uCInterrupt;
}

uint32_t AD5940_MCUResourceInit() {
    Serial.begin(SERIAL_BAUD);

    // Initializing Pins
    SPI.begin(SCK, MISO, MOSI, CS);
    pinMode(CS, OUTPUT);
    pinMode(RESET, OUTPUT);

    /* DOUBLE CHECK IF CHOSEN INTERRUPT CAN HAVE INPUT PULLUP */
    pinMode(ESP32_INTERRUPT, INPUT_PULLUP);
    
    // Initializing Interrupts 
    // AD5940 does FALLING interrupts by default
    attachInterrupt(digitalPinToInterrupt(ESP32_INTERRUPT), interruptISR, FALLING);

    // Sets Reset and CS pins high as a default
    AD5940_RstSet();
    AD5940_CsSet();

    return 0;
}
