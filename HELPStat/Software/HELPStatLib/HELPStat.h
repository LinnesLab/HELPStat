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

extern "C" {
    #include <ad5940.h>
    #include <Impedance.h>
}

/* 
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

    11/03/2023: 
    Removed sequencer functionality. Just need to add sweep and settling times.

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

    11/30/2023:
    Tried with the LPDAC and bias but the oscilloscope gave funky results and clipped signals.
    Removed it from the impedance codes and will test using a separate config and waveform function.
    I want to see if I can't find consistent settings that could hopefully lead to a variable bias 
    and a better waveform output because if it's clipping like the scope showed, could be an area
    of inconsistency. Will test with KFeCN for now with the current bias (1.1V)
    and see what results look like. 

    01/08/2024: Bias works, verified with DMM. Scope is giving mixed results, however.

    02/06/2024: Moved settlingDelay() to after DFT convert starts. Added an empirical delay
    of 500 ms to when WG starts instead. Getting more consistent results this way, I think. 
    Getting an initial negative value for Rimaginary but it's low so likely alright.
    
    02/07/2024: Changed 500 ms to 1s but reverted back because there wasn't really a difference.

    02/08/2024: Need to figure out how to isolate bias. Maybe I don't turn on WG
    until a set delay time passes? 

    02/10/2024: Isolated bias by turning everything but the WG on. That way
    bias is applied but no sine wave is active yet. Will see if this affects results for RC case. 
    Also want to experiment with Gain and Offset Calibration on WG.

    02/11/2024: Reverted to just delaying the initial measurement rather than
    the method described in 02/10/2024. Also added settling delay func to the waveform
    generator, but honestly think that the biggest indicator of accuracy 
    is currently the RTIA. Getting some high imaginary value noise at the last measurement.
    Not sure why. Could be due to delay?

    02/12/2024: Made RCAL an input to TDD - better for long term. 
    Changed constDelay to 1000 instead of 3000 and the delay function
    for frequencies <= 5 Hz to do two full cycles + 2 seconds. I like this better because
    it's shorter and has more rational in letting even slower frequencies wait
    for at least two cycles without compromising the higher frequencies.

    02/13/2024: Added a delay input to runSweep to allow for 
    equilibration if needed. Doing it here when Sleep Mode is disabled
    just in case device goes to sleep mode in between. 

    02/16/2024: Refactored configuration of HSTIA to be an input instead of hard-coding. 
    Gain size needs to be calculated in .ino file rather than the library. Run into bugs otherwise. 
    Seems to work reliably with dummy values. Now to test with a frequency sweep and actual EIS. 
    Should now be easier to alter gains for each measurment.
    
    03/2024: Added current noise function for automatically running measurements for open-circuit noise.

    05/14/2024: Renamed from TestLibImp to HELPStat for consistency with published paper.
*/

#define SYSCLCK 16000000.0  // System Clock frequency (16 MHz)
#define APPBUFF_SIZE 512    // Buffer for impedance, probs don't need this

/* CHANGE ARRAY SIZE TO ACCOMODATE DESIRED NUM OF POINTS */
#define ARRAY_SIZE 200      // Constant for array size of data
#define NOISE_ARRAY 7200

/* Default LPDAC resolution(2.5V internal reference). */
#define DAC12BITVOLT_1LSB   (2200.0f/4095)  //mV
#define DAC6BITVOLT_1LSB    (DAC12BITVOLT_1LSB*64)  //mV

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
        uint32_t _waitClcks; // clock cycles to wait for
        SoftSweepCfg_Type _sweepCfg;
        float _currentFreq; 
        float _startFreq; 
        float _endFreq;
        bool _isSD;

        // Array for EIS data
        impStruct eisArr[ARRAY_SIZE];

        // Keeping track of cycles 
        uint32_t _numCycles; 
        uint32_t _currentCycle;  

        // Gain Calibration Array
        calHSTIA _gainArr[ARRAY_SIZE];
        int _arrHSTIA[ARRAY_SIZE];
        uint32_t _gainArrSize; 

        int _extGain; 
        int _dacGain;
        

        // Bias voltage for LPDAC 
        float _biasVolt; 
        
        // Calibration Resistor 
        float _rcalVal;

        // Noise array
        adcStruct _noiseArr[NOISE_ARRAY];

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
        void AD5940_TDD(float startFreq, float endFreq, uint32_t numPoints, float biasVolt, float zeroVolt, float rcalVal, calHSTIA *gainArr, int gainArrSize, int extGain, int dacGain); // works
        void AD5940_DFTMeasure(void); // works
        void pollDFT(int32_t* pReal, int32_t* pImage); // works
        void getDFT(int32_t* pReal, int32_t* pImage); // works

        /* Helper Functions */
        void getMagPhase(int32_t real, int32_t image, float* pMag, float* pPhase); // works 
        void logSweep(SoftSweepCfg_Type *pSweepCfg, float *pNextFreq); // works
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
};  

#endif