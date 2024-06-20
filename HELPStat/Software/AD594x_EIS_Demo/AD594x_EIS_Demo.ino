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

// #include <BLEDevice.h>
// #include <BLEServer.h>
// #include <BLEUtils.h>
// #include <BLE2902.h>

#include <vector>
#include <string>

// #define SERVICE_UUID                    "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
// #define CHARACTERISTIC_UUID_START       "beb5483e-36e1-4688-b7f5-ea07361b26a8"
// #define CHARACTERISTIC_UUID_RCT         "a5d42ee9-0551-4a23-a1b7-74eea28aa083"
// #define CHARACTERISTIC_UUID_RS          "192fa626-1e5a-4018-8176-5debff81a6c6"
// #define CHARACTERISTIC_UUID_NUMCYCLES   "8117179a-b8ee-433c-96da-65816c5c92dd"
// #define CHARACTERISTIC_UUID_NUMPOINTS   "359a6d93-9007-41f6-bbbe-f92bc17db383"
// #define CHARACTERISTIC_UUID_STARTFREQ   "5b0210d0-cd21-4011-9882-db983ba7e1fc"
// #define CHARACTERISTIC_UUID_ENDFREQ     "3507abdc-2353-486b-a3d5-dd831ee4bb18"
// #define CHARACTERISTIC_UUID_RCALVAL     "4f7d237e-a358-439e-8771-4ab7f81473fa"
// #define CHARACTERISTIC_UUID_BIASVOLT    "62df1950-23f9-4acd-8473-61a421d4cf07"
// #define CHARACTERISTIC_UUID_ZEROVOLT    "60d57f7b-6e41-41e5-bd44-0e23638e90d2"
// #define CHARACTERISTIC_UUID_DELAYSECS   "57a7466e-c0e1-4f6e-aea4-99ef4f360d24"
// #define CHARACTERISTIC_UUID_EXTGAIN     "e17e690a-16e8-4c70-b958-73e41d4afff0"
// #define CHARACTERISTIC_UUID_DACGAIN     "36377d50-6ba7-4cc1-825a-42746c4028dc"
// #define CHARACTERISTIC_UUID_FOLDERNAME  "02193c1e-4afe-4211-b64f-e878e9d6c0a4"
// #define CHARACTERISTIC_UUID_FILENAME    "d07519f0-1c45-461a-9b8e-fcaad4e53f0c"

// BLEServer* pServer = NULL;
// BLECharacteristic* pCharacteristicStart      = NULL;
// BLECharacteristic* pCharacteristicRct        = NULL;
// BLECharacteristic* pCharacteristicRs         = NULL;
// BLECharacteristic* pCharacteristicNumCycles  = NULL;
// BLECharacteristic* pCharacteristicNumPoints  = NULL;
// BLECharacteristic* pCharacteristicStartFreq  = NULL;
// BLECharacteristic* pCharacteristicEndFreq    = NULL;
// BLECharacteristic* pCharacteristicRcalVal    = NULL;
// BLECharacteristic* pCharacteristicBiasVolt   = NULL;
// BLECharacteristic* pCharacteristicZeroVolt   = NULL;
// BLECharacteristic* pCharacteristicDelaySecs  = NULL;
// BLECharacteristic* pCharacteristicExtGain    = NULL;
// BLECharacteristic* pCharacteristicDacGain    = NULL;
// BLECharacteristic* pCharacteristicFolderName = NULL;
// BLECharacteristic* pCharacteristicFileName   = NULL;

// bool deviceConnected = false;
// bool start_value     = false;
// bool old_start_value = false;

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

float startFreq = 100000; 
float endFreq = 1; 
float biasVolt = 0.0; 
float zeroVolt = 0.0; 
float rcalVal = 1000; // Use the measured resistance of the chosen calibration resistor

int extGain = 1; 
int dacGain = 1; 

float rct_estimate = 127000;
float rs_estimate = 150;

HELPStat demo;

  //void setup_bluetooth(void) {
  // Create the BLE Device
  // BLEDevice::init("HELPStat");

  // // Create the BLE Server
  // pServer = BLEDevice::createServer();
  // pServer->setCallbacks(new MyServerCallbacks());

  // // Create the BLE Service
  // BLEService *pService = pServer->createService(BLEUUID(SERVICE_UUID),45,0);

  // // Create a BLE Characteristic
  // pCharacteristicStart = pService->createCharacteristic(
  //                     CHARACTERISTIC_UUID_START,
  //                     BLECharacteristic::PROPERTY_WRITE  |
  //                     BLECharacteristic::PROPERTY_NOTIFY
  //                   );
  
  // pCharacteristicRct = pService->createCharacteristic(
  //                     CHARACTERISTIC_UUID_RCT,
  //                     BLECharacteristic::PROPERTY_READ   |
  //                     BLECharacteristic::PROPERTY_NOTIFY |
  //                     BLECharacteristic::PROPERTY_WRITE
  //                   );
  // pCharacteristicRs = pService->createCharacteristic(
  //                     CHARACTERISTIC_UUID_RS,
  //                     BLECharacteristic::PROPERTY_READ   |
  //                     BLECharacteristic::PROPERTY_NOTIFY |
  //                     BLECharacteristic::PROPERTY_WRITE
  //                   );

  // pCharacteristicNumCycles = pService->createCharacteristic(
  //                     CHARACTERISTIC_UUID_NUMCYCLES,
  //                     BLECharacteristic::PROPERTY_WRITE |
  //                     BLECharacteristic::PROPERTY_NOTIFY
  //                   );

  // pCharacteristicNumPoints = pService->createCharacteristic(
  //                     CHARACTERISTIC_UUID_NUMPOINTS,
  //                     BLECharacteristic::PROPERTY_WRITE |
  //                     BLECharacteristic::PROPERTY_NOTIFY
  //                   );

  // pCharacteristicStartFreq = pService->createCharacteristic(
  //                     CHARACTERISTIC_UUID_STARTFREQ,
  //                     BLECharacteristic::PROPERTY_WRITE |
  //                     BLECharacteristic::PROPERTY_NOTIFY
  //                   );

  // pCharacteristicEndFreq = pService->createCharacteristic(
  //                     CHARACTERISTIC_UUID_ENDFREQ,
  //                     BLECharacteristic::PROPERTY_WRITE |
  //                     BLECharacteristic::PROPERTY_NOTIFY
  //                   ); 

  // pCharacteristicRcalVal = pService->createCharacteristic(
  //                     CHARACTERISTIC_UUID_RCALVAL,
  //                     BLECharacteristic::PROPERTY_WRITE |
  //                     BLECharacteristic::PROPERTY_NOTIFY
  //                   );

  // pCharacteristicBiasVolt = pService->createCharacteristic(
  //                     CHARACTERISTIC_UUID_BIASVOLT,
  //                     BLECharacteristic::PROPERTY_WRITE |
  //                     BLECharacteristic::PROPERTY_NOTIFY
  // );

  // pCharacteristicZeroVolt = pService->createCharacteristic(
  //                     CHARACTERISTIC_UUID_ZEROVOLT,
  //                     BLECharacteristic::PROPERTY_WRITE |
  //                     BLECharacteristic::PROPERTY_NOTIFY
  // );

  // pCharacteristicDelaySecs = pService->createCharacteristic(
  //                     CHARACTERISTIC_UUID_DELAYSECS,
  //                     BLECharacteristic::PROPERTY_WRITE |
  //                     BLECharacteristic::PROPERTY_NOTIFY
  // );

  // pCharacteristicExtGain = pService->createCharacteristic(
  //                     CHARACTERISTIC_UUID_EXTGAIN,
  //                     BLECharacteristic::PROPERTY_WRITE |
  //                     BLECharacteristic::PROPERTY_NOTIFY
  // );

  // pCharacteristicDacGain = pService->createCharacteristic(
  //                     CHARACTERISTIC_UUID_DACGAIN,
  //                     BLECharacteristic::PROPERTY_WRITE |
  //                     BLECharacteristic::PROPERTY_NOTIFY
  // );

  // pCharacteristicFolderName = pService->createCharacteristic(
  //                     CHARACTERISTIC_UUID_FOLDERNAME,
  //                     BLECharacteristic::PROPERTY_WRITE |
  //                     BLECharacteristic::PROPERTY_NOTIFY
  // );

  // pCharacteristicFileName = pService->createCharacteristic(
  //                     CHARACTERISTIC_UUID_FILENAME,
  //                     BLECharacteristic::PROPERTY_WRITE |
  //                     BLECharacteristic::PROPERTY_NOTIFY
  // );


  // // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
  // // Create a BLE Descriptor
  // pCharacteristicStart->addDescriptor(new BLE2902());
  // pCharacteristicRct->addDescriptor(new BLE2902());
  // pCharacteristicRs->addDescriptor(new BLE2902());
  // pCharacteristicNumCycles->addDescriptor(new BLE2902());
  // pCharacteristicNumPoints->addDescriptor(new BLE2902());
  // pCharacteristicStartFreq->addDescriptor(new BLE2902());
  // pCharacteristicEndFreq->addDescriptor(new BLE2902());
  // pCharacteristicRcalVal->addDescriptor(new BLE2902());

  // pCharacteristicBiasVolt->addDescriptor(new BLE2902());
  // pCharacteristicZeroVolt->addDescriptor(new BLE2902());
  // pCharacteristicDelaySecs->addDescriptor(new BLE2902());
  // pCharacteristicExtGain->addDescriptor(new BLE2902());
  // pCharacteristicDacGain->addDescriptor(new BLE2902());
  // pCharacteristicFolderName->addDescriptor(new BLE2902());
  // pCharacteristicFileName->addDescriptor(new BLE2902());

  // // Start the service
  // pService->start();

  // // Start advertising
  // BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  // pAdvertising->addServiceUUID(SERVICE_UUID);
  // pAdvertising->setScanResponse(false);
  // pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  // pAdvertising->setMinPreferred(0x12);
  // BLEDevice::startAdvertising();
  // Serial.println("Waiting a client connection to notify...");
  //}

/* Initializing Analog Pins and Potentiostat pins */
String folderName = "folder-name-here"; 
String fileName = "file-name-here"; 

void setup() {
  // Serial.begin(115200);

  // // Timer for Bluetooth Communication
  // setup_bluetooth();
  
  demo.BLE_setup();
  /* Optional inputs to establish pins for button and LEDs*/
  pinMode(LEDPIN, OUTPUT);
  pinMode(BUTTON, INPUT);
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

  /* Testing ADC Voltage Readings */

//  demo.AD5940_TDDNoise(0.0, 0.0); // Using default 1.11V reference
//  demo.AD5940_ADCMeasure();

    /* Noise Measurements*/
//    delay(2000);
//    demo.ADCNoiseTest();
//    demo.AD5940_HSTIARcal(HSTIARTIA_160K, 149700); 

    /* If you want to save noise data - optional but might contribute to noise */
//    demo.saveDataNoise("03-21-24", "5k-ohms");
}

void loop() {
  // put your main code here, to run repeatedly: 
  demo.BLE_settings();
  int buttonStatus = digitalRead(BUTTON); 
  delay(10); // Additional Debounce delay - 10 ms 

  // if(buttonStatus == LOW)
  // {
  digitalWrite(LEDPIN, LOW);
  delay(1000);
  Serial.println("Button pressed - starting measurements!");

  demo.print_settings();

  // blinkLED(3, 0);
    
  /* Main Testing Code - also used for current draw as a standard sweep measurement */
  demo.AD5940_TDD(test, gainSize); // This version uses the private variables for startFreq, endFreq, etc.
  demo.runSweep();
  std::vector<float> resistors = demo.calculateResistors();
  demo.BLE_transmitResults();
  demo.saveDataEIS("folder-name-here", "file-name-here");
  


    /* Current Draw Code - (no sweep but set for measurement)*/
  // demo.AD5940_TDD(startFreq, endFreq, 6, 0.0, 0.0, 9870, test, gainSize, 1, 1);
  // demo.configureFrequency(endFreq);

    /* Main Testing Code - also used for current draw as a standard sweep measurement */
    /* Start, Stop, NumPoints, Vbias, Vzero, Rcal, gain array, gain size, Excitation Gain, DAC Gain*/
    // demo.AD5940_TDD(startFreq, endFreq, numPoints, biasVolt, zeroVolt, rcalVal, test, gainSize, extGain, dacGain);
    // demo.runSweep(numCycles, delaySecs); // Run the Sweep
    // demo.saveDataEIS(folderName, fileName);

    /* After Impedance Measurement - drive LED High and get ready to restart measurement */
  delay(500);
  blinkLED(0, 1); 
  Serial.println("Ready to go again!");
  // }
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
