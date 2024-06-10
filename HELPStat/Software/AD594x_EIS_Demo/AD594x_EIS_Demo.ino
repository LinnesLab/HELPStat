/*
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

#include "HELPStat.h"

/* Reference constants for gain values - taken from AD5941.h library by Analog Devices */
//#define HSTIARTIA_200               0     /**< HSTIA Internal RTIA resistor 200  */
//#define HSTIARTIA_1K                1     /**< HSTIA Internal RTIA resistor 1K   */
//#define HSTIARTIA_5K                2     /**< HSTIA Internal RTIA resistor 5K   */
//#define HSTIARTIA_10K               3     /**< HSTIA Internal RTIA resistor 10K  */
//#define HSTIARTIA_20K               4     /**< HSTIA Internal RTIA resistor 20K  */
//#define HSTIARTIA_40K               5     /**< HSTIA Internal RTIA resistor 40K  */
//#define HSTIARTIA_80K               6     /**< HSTIA Internal RTIA resistor 80K  */
//#define HSTIARTIA_160K              7     /**< HSTIA Internal RTIA resistor 160K */
//#define HSTIARTIA_OPEN              8     /**< Open internal resistor */

//#define EXCITBUFGAIN_2              0   /**< Excitation buffer gain is x2 */
//#define EXCITBUFGAIN_0P25           1   /**< Excitation buffer gain is x1/4 */

//#define HSDACGAIN_1                 0   /**< Gain is x1 */
//#define HSDACGAIN_0P2               1   /**< Gain is x1/5 */

/* Using I2C pins as GPIOs for buttons - Optional */
#define BUTTON 7
#define LEDPIN 6

calHSTIA test[] = {          
  {0.51,  HSTIARTIA_40K},       
  {1.5,         HSTIARTIA_10K},
  {20,      HSTIARTIA_5K},
  {150,       HSTIARTIA_5K},
  {400,         HSTIARTIA_1K},
  {100000,     HSTIARTIA_200}  
};    
 
/* Variables for function inputs */
int gainSize = (int)sizeof(test) / sizeof(test[0]);

uint32_t numCycles = 0; 
uint32_t delaySecs = 0; 
uint32_t numPoints = 6; 

float startFreq = 100; 
float endFreq = 0.15; 
float biasVolt = 0.0; 
float zeroVolt = 0.0; 
float rcalVal = 9870; // Use the measured resistance of the chosen calibration resistor

int extGain = 1; 
int dacGain = 1; 

HELPStat demo;

void setup() {
  
  /* Optional inputs to establish pins for button and LEDs*/
  pinMode(BUTTON, INPUT); 
  pinMode(LEDPIN, OUTPUT);
  digitalWrite(LEDPIN, HIGH); // Initial LED State
  
  /* 
   *  Initializing Analog Pins and Potentiostat pins - need this function 
   *  regardless of running Analog Devices or our EIS code.
  */
  demo.AD5940Start();

  // Configuration is complete
  Serial.println("Pins configured! Press button to begin.");

  /* Main Testing Code - run these functions if you want to run without the loop */
//  demo.AD5940_TDD(100000, 0.1, 6, 0.0, 0.0, 9870, test, gainSize, 1, 1);
//  Serial.print("Calibration size: ");
//  Serial.println(gainSize);
  
  /* Testing Sweep - needs AD5940_TDD to be uncommented too */
//  demo.runSweep(0, 0);

  /* Testing SD card */
//  demo.printData(); `
//  demo.saveDataEIS("EIS data", "03-02-24-pc-ph-e2-5mM-kfecn-pbs-1Mtie");

  /* 
   *  Running Analog Devices Code for Impedance - adapted to use the gain array. 
   *  Note: this runs indefinitely. Comment out all the other demo functions 
   *  exceot AD5940_Start before running this. 
  */
  
  /* Start, Stop, Num points per decade, gain size, and gain array */
//    demo.AD5940_Main(100000, 0.1, 6, gainSize, test);
}

void loop() {
  // put your main code here, to run repeatedly:
  // Reading the button state 
  int buttonStatus = digitalRead(BUTTON); 
  delay(10); // Additional Debounce delay - 10 ms 

  if(buttonStatus == LOW)
  {
    digitalWrite(LEDPIN, LOW);
    delay(1000);
    Serial.println("Button pressed - starting measurements!");
    blinkLED(3, 0);
    
    /* Main Testing Code - also used for current draw as a standard sweep measurement */
    /* Start, Stop, NumPoints, Vbias, Vzero, Rcal, gain array, gain size, Excitation Gain, DAC Gain*/
    demo.AD5940_TDD(startFreq, endFreq, numPoints, biasVolt, zeroVolt, rcalVal, test, gainSize, extGain, dacGain);
    demo.runSweep(numCycles, delaySecs); // Run the Sweep
    demo.saveDataEIS("folder-name-here", "file-name-here");

    /* After Impedance Measurement - drive LED High and get ready to restart measurement */
    delay(500);
    blinkLED(0, 1); 
    Serial.println("Ready to go again!");
  }
}

void blinkLED(int cycles, bool state) {
  if(state) 
  {
    digitalWrite(LEDPIN, HIGH);
    delay(10);
  }
  else
  {
    for(int i = 0; i < cycles; i++)
    {
      digitalWrite(LEDPIN, HIGH); 
      delay(1000); 
      digitalWrite(LEDPIN, LOW);
      delay(1000);
    }
  }
}
