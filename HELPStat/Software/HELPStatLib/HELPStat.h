/*
    FILENAME: HELPStat.h
    AUTHOR: Kevin Alessandro Bautista
    EMAIL: kbautis@purdue.edu

    DISCLAIMER: 
    Linnes Lab code, firmware, and software is released under the MIT License
    (http://opensource.org/licenses/MIT).
    
    The MIT License (MIT)
    
    Copyright (c) 2024 Linnes Lab, Purdue University, West Lafayette, IN, USA
    
    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights to
    use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
    of the Software, and to permit persons to whom the Software is furnished to do
    so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

#ifndef HELPSTAT_H
#define HELPSTAT_H

// Imports 
#include "Arduino.h"
#include <constants.h>

// SD Card Functionality
#include "SD.h"
#include "FS.h"

// Levenberg-Marquardt Functionality
#include "lma.h"

// BLE
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

extern "C" {
    #include <ad5940.h>
    #include <Impedance.h>
}

/* 
    06/13/2024: Shannon Riegle here. Implemented BLE communication to allow user to reconfigure different
    settings w/o needing to reprogram the HELPStat PCB each time. Also reorganized data-logging to have
    most recent logs on top. Figured this would be easier for future developers to see what the most recent
    changes to the library were.

    Also had the Levenberg-Marquardt fitting algorithm moved into this library. This is dependent on another
    C++ library called Eigen. I modified Bolder Flight System's Eigen port to include the unsupported features
    seen on the computer version, namely nonlinear solving. More documentation on that can be found at:
    https://github.com/LinnesLab/Eigen-Port
    
    05/14/2024: Renamed from TestLibImp to HELPStat for consistency with published paper.

    03/2024: Added current noise function for automatically running measurements for open-circuit noise.
    
    02/16/2024: Refactored configuration of HSTIA to be an input instead of hard-coding. 
    Gain size needs to be calculated in .ino file rather than the library. Run into bugs otherwise. 
    Seems to work reliably with dummy values. Now to test with a frequency sweep and actual EIS. 
    Should now be easier to alter gains for each measurment.
    
    02/13/2024: Added a delay input to runSweep to allow for 
    equilibration if needed. Doing it here when Sleep Mode is disabled
    just in case device goes to sleep mode in between. 
    
    02/12/2024: Made RCAL an input to TDD - better for long term. 
    Changed constDelay to 1000 instead of 3000 and the delay function
    for frequencies <= 5 Hz to do two full cycles + 2 seconds. I like this better because
    it's shorter and has more rational in letting even slower frequencies wait
    for at least two cycles without compromising the higher frequencies.

    02/11/2024: Reverted to just delaying the initial measurement rather than
    the method described in 02/10/2024. Also added settling delay func to the waveform
    generator, but honestly think that the biggest indicator of accuracy 
    is currently the RTIA. Getting some high imaginary value noise at the last measurement.
    Not sure why. Could be due to delay?

    02/10/2024: Isolated bias by turning everything but the WG on. That way
    bias is applied but no sine wave is active yet. Will see if this affects results for RC case. 
    Also want to experiment with Gain and Offset Calibration on WG.
    
    02/08/2024: Need to figure out how to isolate bias. Maybe I don't turn on WG
    until a set delay time passes?
    
    02/07/2024: Changed 500 ms to 1s but reverted back because there wasn't really a difference.
    
    02/06/2024: Moved settlingDelay() to after DFT convert starts. Added an empirical delay
    of 500 ms to when WG starts instead. Getting more consistent results this way, I think. 
    Getting an initial negative value for Rimaginary but it's low so likely alright.
    
    01/08/2024: Bias works, verified with DMM. Scope is giving mixed results, however.
    
    11/30/2023:
    Tried with the LPDAC and bias but the oscilloscope gave funky results and clipped signals.
    Removed it from the impedance codes and will test using a separate config and waveform function.
    I want to see if I can't find consistent settings that could hopefully lead to a variable bias 
    and a better waveform output because if it's clipping like the scope showed, could be an area
    of inconsistency. Will test with KFeCN for now with the current bias (1.1V)
    and see what results look like. 

    11/06/2023:
    Looks like the issue was primarily the HSTIA (go figure) values. Set them 
    appropriately and getting decent data. Problem is that high frequency 
    around 100kHz to about 20kHz is relatively noisy, and low frequency (< 0.5 Hz) is too. 

    Will try increasing the settling time for the low frequency runs < 0.5 Hz to double the period
    + a constant. Unsure of what to do for the high frequency noise. I want to try 
    adding a delay between cycles because I think it may switch too fast after calibration? 
    Is that a thing? 
    
    But overall, getting decent enough data with the sweep as it is, and 
    about a few minutes faster than the normal method. This also gives me more control about 
    what to do with the data afterwards. Can probably start testing with the impedance board like this
    since I'll keep optimizing it anyways. Will be a nice benchmark to compare with the 
    Impedance.c method that utilizes the sequencer. 

    11/05/2023: 
    Added sweep functionality and also implemented repeating cycles in the code. 
    Just need to figure out how to best add a delay for settling time without it taking 
    forever. 

    I think modern potentiostats do this by having it dependent on frequency, and 
    so I'll probably do 2/freq + a constant wait time so that I'm waiting for at least
    two periods and see if that works. Also probably need to make it more robust 
    by adding a configuration and DFT struct to enable user inputs. 

    11/05/2023:
    2 / freq + const isn't working. Will add a check frequency function
    to see if I can at least get high frequencies working. I'm not sure what
    is a good enough range to wait for the settling time.

    11/03/2023: 
    Removed sequencer functionality. Just need to add sweep and settling times.

    09/21/2023:
    Functions utilized are based on Analog Devices examples. 
    Impedance.h and Impedance.c were used to help develop our own version
    of impedance measurements and serve as a reference.
    
    Structure for interfacing with AD5941 is based on existing FreiStat port
    but extended for the ESP32.

    This library primarily exists to validate the impedance measurement
    functionality of the AD5941 and serve as a benchmark for comparison
    when creating my own library.

    The current state uses the sequencer but the goal is to ultimately 
    use it without the Sequencer to have better control in terms of signal / 
    measurement delay. At least I think using it without it is better.

    Big change - Interrupt pin is intialized to INPUT_PULLUP. I think 
    this may have been a big factor in defining an initial state for falling.
    Or not. We'll see. 
*/

#define SYSCLCK 16000000.0  // System Clock frequency (16 MHz)
#define APPBUFF_SIZE 512    // Buffer for impedance, probs don't need this

/* CHANGE ARRAY SIZE TO ACCOMODATE DESIRED NUM OF POINTS */
#define ARRAY_SIZE 200      // Constant for array size of data
#define NOISE_ARRAY 7200

/* Default LPDAC resolution(2.5V internal reference). */
#define DAC12BITVOLT_1LSB   (2200.0f/4095)  //mV
#define DAC6BITVOLT_1LSB    (DAC12BITVOLT_1LSB*64)  //mV

// Push Button
#define BUTTON 7


// BLE Characteristics
#define SERVICE_UUID                    "4fafc201-1fb5-459e-8fcc-c5c9c331914b" // Device UUID
#define CHARACTERISTIC_UUID_START       "beb5483e-36e1-4688-b7f5-ea07361b26a8" // UUIDs for different parameters
#define CHARACTERISTIC_UUID_RCT         "a5d42ee9-0551-4a23-a1b7-74eea28aa083"
#define CHARACTERISTIC_UUID_RS          "192fa626-1e5a-4018-8176-5debff81a6c6"
#define CHARACTERISTIC_UUID_NUMCYCLES   "8117179a-b8ee-433c-96da-65816c5c92dd"
#define CHARACTERISTIC_UUID_NUMPOINTS   "359a6d93-9007-41f6-bbbe-f92bc17db383"
#define CHARACTERISTIC_UUID_STARTFREQ   "5b0210d0-cd21-4011-9882-db983ba7e1fc"
#define CHARACTERISTIC_UUID_ENDFREQ     "3507abdc-2353-486b-a3d5-dd831ee4bb18"
#define CHARACTERISTIC_UUID_RCALVAL     "4f7d237e-a358-439e-8771-4ab7f81473fa"
#define CHARACTERISTIC_UUID_BIASVOLT    "62df1950-23f9-4acd-8473-61a421d4cf07"
#define CHARACTERISTIC_UUID_ZEROVOLT    "60d57f7b-6e41-41e5-bd44-0e23638e90d2"
#define CHARACTERISTIC_UUID_DELAYSECS   "57a7466e-c0e1-4f6e-aea4-99ef4f360d24"
#define CHARACTERISTIC_UUID_EXTGAIN     "e17e690a-16e8-4c70-b958-73e41d4afff0"
#define CHARACTERISTIC_UUID_DACGAIN     "36377d50-6ba7-4cc1-825a-42746c4028dc"
#define CHARACTERISTIC_UUID_FOLDERNAME  "02193c1e-4afe-4211-b64f-e878e9d6c0a4"
#define CHARACTERISTIC_UUID_FILENAME    "d07519f0-1c45-461a-9b8e-fcaad4e53f0c"

typedef struct _impStruct {
    float freq;
    float magnitude; 
    float phaseRad; 
    float real;
    float imag; 
    float phaseDeg; 
}impStruct;

typedef struct _calHSTIA
{
    float freq; 
    int rTIA; 
}calHSTIA; 

typedef struct _adcStruct {
    unsigned long interval;
    uint32_t idx; 
    float vSE0; 
    float vRE0;
    float diff; 
    float diffInv; 
}adcStruct;

class HELPStat {
    private:
        class MyServerCallbacks: public BLEServerCallbacks { // BLE Callback (just says "Device Connected" or "Device Disconnected")
            void onConnect(BLEServer* pServer) {
                bool deviceConnected = true;
                Serial.println("Device Connected");
            };

            void onDisconnect(BLEServer* pServer) {
                bool deviceConnected = false;
                pServer->startAdvertising();
                Serial.println("Device Disconnected");
            };
        };

        uint32_t _waitClcks; // clock cycles to wait for
        SoftSweepCfg_Type _sweepCfg;
        float _currentFreq; 
        float _startFreq = 100000; // Initialize w/ default values 
        float _endFreq   = 1;
        bool _isSD;

        // Rct/Rs estimates
        float _rct_estimate = 5000; // Initialize w/ default values 
        float _rs_estimate  = 50;

        // Array for EIS data
        impStruct eisArr[ARRAY_SIZE];

        // Keeping track of cycles 
        uint32_t _numCycles = 1; // Initialize w/ default values  
        uint32_t _currentCycle;
        uint32_t _numPoints = 6; // Initialize w/ default values 

        // Gain Calibration Array
        calHSTIA _gainArr[ARRAY_SIZE];
        int _arrHSTIA[ARRAY_SIZE];
        uint32_t _gainArrSize; 

        int _extGain = 1; // Initialize w/ default values 
        int _dacGain = 1;

        // Bias voltage for LPDAC 
        float _biasVolt = 0.0;  // Initialize w/ default values  
        float _zeroVolt = 0.0;
        
        // Calibration Resistor 
        float _rcalVal  = 1000; // Initialize w/ default values 

        // Delay
        uint32_t _delaySecs = 0.0; // Initialize w/ default values 

        // Calculated Rct/Rs
        float _calculated_Rct;
        float _calculated_Rs;     

        // Noise array
        adcStruct _noiseArr[NOISE_ARRAY];

        // Bluetooth Characteristics
        BLEServer* pServer = NULL;
        BLECharacteristic* pCharacteristicStart      = NULL;
        BLECharacteristic* pCharacteristicRct        = NULL;
        BLECharacteristic* pCharacteristicRs         = NULL;
        BLECharacteristic* pCharacteristicNumCycles  = NULL;
        BLECharacteristic* pCharacteristicNumPoints  = NULL;
        BLECharacteristic* pCharacteristicStartFreq  = NULL;
        BLECharacteristic* pCharacteristicEndFreq    = NULL;
        BLECharacteristic* pCharacteristicRcalVal    = NULL;
        BLECharacteristic* pCharacteristicBiasVolt   = NULL;
        BLECharacteristic* pCharacteristicZeroVolt   = NULL;
        BLECharacteristic* pCharacteristicDelaySecs  = NULL;
        BLECharacteristic* pCharacteristicExtGain    = NULL;
        BLECharacteristic* pCharacteristicDacGain    = NULL;
        BLECharacteristic* pCharacteristicFolderName = NULL;
        BLECharacteristic* pCharacteristicFileName   = NULL;

        // bool deviceConnected = false;
        bool start_value     = false;
        bool old_start_value = false;

        // File and FolderNames
        String folderName = "folder-name-here"; 
        String fileName = "file-name-here"; 

    public:
        HELPStat();
        AD5940Err AD5940Start(void); 

        /* Sequencer methods from Analog Devices */
        int32_t AD5940PlatformCfg(void);
        void AD5940ImpedanceStructInit(float startFreq, float endFreq, uint32_t numPoints);
        int32_t ImpedanceShowResult(uint32_t *pData, uint32_t DataCount);
        void DFTPolling_Main(void);
        void AD5940_Main(float startFreq, float endFreq, uint32_t numPoints, uint32_t gainArrSize, calHSTIA* gainArr);

        /* 10-27-2023 - IMPEDANCE NO SEQUENCER - our methods for testing */
        // NOTE: Two overloads
        void AD5940_TDD(calHSTIA *gainArr, int gainArrSize);
        void AD5940_TDD(float startFreq, float endFreq, uint32_t numPoints, float biasVolt, float zeroVolt, float rcalVal, calHSTIA *gainArr, int gainArrSize, int extGain, int dacGain); // works
        
        void AD5940_DFTMeasure(void); // works
        void pollDFT(int32_t* pReal, int32_t* pImage); // works
        void getDFT(int32_t* pReal, int32_t* pImage); // works

        /* Helper Functions */
        void getMagPhase(int32_t real, int32_t image, float* pMag, float* pPhase); // works 
        void logSweep(SoftSweepCfg_Type *pSweepCfg, float *pNextFreq); // works
        void runSweep(void);
        void runSweep(uint32_t numCycles, uint32_t delaySecs); // sweep works now and cycles correctly 
        void resetSweep(SoftSweepCfg_Type *pSweepCfg, float *pNextFreq); // works
        
        /* Both these functions need better optimization but they work for now */
        void settlingDelay(float freq);
        AD5940Err checkFreq(float freq);

        /* SD Card Functions */
        void sdWrite(char *output);
        void sdAppend(char *output);
        void printData(void); 
        void saveDataEIS(String dirName, String fileName);

        /* LMA for Rct / Rs Calculation */
        std::vector<float> calculateResistors(void);
        std::vector<float> calculateResistors(float rct_estimate, float rs_estimate);

        /* Functions to test bias voltage */
        void AD5940_BiasCfg(float startFreq, float endFreq, uint32_t numPoints, float biasVolt, float zeroVolt, int delaySecs);

        /* Function to test EIS method */
        void AD5940_DFTMeasureEIS(void);

        /* Functions to better adjust HSTIA and settings */
        void configureDFT(float freq);
        AD5940Err setHSTIA(float freq);
        void configureFrequency(float freq);

        /* Current noise measurements */
        void AD5940_TDDNoise(float biasVolt, float zeroVolt);
        float pollADC(uint32_t gainPGA, float vRef1p82);
        float getADCVolt(uint32_t gainPGA, float vRef1p82); 
        void AD5940_ADCMeasure(void);
        void ADCsweep(void);
        void ADCNoiseTest(void);
        void PGACal(void);

        void saveDataNoise(String dirName, String fileName);
        void AD5940_HSTIARcal(int rHSTIA, float rcalVal);

        void BLE_setup(void);
        void BLE_settings(void);
        void BLE_transmitResistors(void);

        void print_settings(void);       
};  

#endif