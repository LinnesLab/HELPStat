#include "HELPStat.h"

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#include <vector>

#define SERVICE_UUID              "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID_START "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define CHARACTERISTIC_UUID_RCT   "a5d42ee9-0551-4a23-a1b7-74eea28aa083"
#define CHARACTERISTIC_UUID_RS    "192fa626-1e5a-4018-8176-5debff81a6c6"

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristicStart = NULL;
BLECharacteristic* pCharacteristicRct   = NULL;
BLECharacteristic* pCharacteristicRs    = NULL;
bool deviceConnected = false;
bool oldDeviceConnected = false;
bool start_value     = false;
bool old_start_value = false;

hw_timer_t *bluetooth_timer = NULL;

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
///* HSDAC PGA Gain selection(DACCON.BIT0) */
//#define HSDACGAIN_1                 0   /**< Gain is x1 */
//#define HSDACGAIN_0P2               1   /**< Gain is x1/5 */

#define BUTTON 7
#define LEDPIN 6

calHSTIA test[] = {          
//  {0.51  HSTIARTIA_40K}       
  {1.5,         HSTIARTIA_10K},
  {20,      HSTIARTIA_5K},
  {150,       HSTIARTIA_5K},
  {400,         HSTIARTIA_1K},
  {100000,     HSTIARTIA_200}  
};    
 
// Variables for function inputs 
int gainSize = (int)sizeof(test) / sizeof(test[0]);

uint32_t numCycles = 0; 
uint32_t delaySecs = 0; 
uint32_t numPoints = 6; 

float startFreq = 100; 
float endFreq = 0.1; 
float biasVolt = 0.0; 
float zeroVolt = 0.0; 
float rcalVal = 9870; // Measured resistance of 10k resistor: 9.87k

int extGain = 1; 
int dacGain = 1; 

float rct_estimate = 10000;
float rs_estimate = 500;

HELPStat demo;

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      deviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      deviceConnected = false;
    }
};

void IRAM_ATTR bluetooth(void) {
  if (!deviceConnected && oldDeviceConnected) {
    delay(500); // give the bluetooth stack the chance to get things ready
    pServer->startAdvertising(); // restart advertising
    Serial.println("start advertising");
    oldDeviceConnected = deviceConnected;
  }
  // connecting
  if (deviceConnected && !oldDeviceConnected) {
    // do stuff here on connecting
    oldDeviceConnected = deviceConnected;
  }
}

void setup_bluetooth(void) {
  // Create the BLE Device
  BLEDevice::init("HELPStat");

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service
  BLEService *pService = pServer->createService(SERVICE_UUID);

  // Create a BLE Characteristic
  pCharacteristicStart = pService->createCharacteristic(
                      CHARACTERISTIC_UUID_START,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_WRITE  |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_INDICATE
                    );
  
  pCharacteristicRct = pService->createCharacteristic(
                      CHARACTERISTIC_UUID_RCT,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_WRITE
                    );
  pCharacteristicRs = pService->createCharacteristic(
                      CHARACTERISTIC_UUID_RS,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_WRITE
                    );

  // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
  // Create a BLE Descriptor
  pCharacteristicStart->addDescriptor(new BLE2902());
  pCharacteristicRct->addDescriptor(new BLE2902());
  pCharacteristicRs->addDescriptor(new BLE2902());

  // Start the service
  pService->start();

  // Start advertising
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  BLEDevice::startAdvertising();
  Serial.println("Waiting a client connection to notify...");
}

void setup() {
  Serial.begin(115200);

  // Timer for Bluetooth Communication
  setup_bluetooth();
  bluetooth_timer = timerBegin(0, 4000, true);
  timerAttachInterrupt(bluetooth_timer, &bluetooth, true);
  timerAlarmWrite(bluetooth_timer, 2000, true);
  timerAlarmEnable(bluetooth_timer);

  /* Initializing Analog Pins and Potentiostat pins */
  pinMode(BUTTON, INPUT); 
  pinMode(LEDPIN, OUTPUT);
  digitalWrite(LEDPIN, HIGH); // Initial LED State
  demo.AD5940Start();
  Serial.println("Pins configured! Press button to begin.");
  

  /* Main Testing Code */
//  demo.AD5940_TDD(100000, 0.1, 6, 0.0, 0.0, 9870, test, gainSize, 1, 1);
//  Serial.print("Calibration size: ");
//  Serial.println(gainSize);

  /* Testing Bias and Single Shot Frequency */
//  demo.AD5940_BiasCfg(100000, 0.1, 6, -100.0, 0.0, 0);
  
  /* Testing Sweep */
//  demo.runSweep(0, 0);

  /* Testing SD card */
//  demo.printData(); `
//  demo.saveDataEIS("EIS data", "03-02-24-pc-ph-e2-5mM-kfecn-pbs-1Mtie");

  /* Testing the HSTIA Function */
//  delay(5000);
//  for(uint32_t i = 0; i < sizeof(freqArr) / sizeof(freqArr[0]); i++)
//  {
//    demo.configureFrequency(freqArr[i]);
//  }

  /* Running Analog Devices Code for Impedance */
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
  // Reading the button state (this will probably be moved to a function or library in the future)
  do{
    old_start_value = start_value;
    start_value = *(pCharacteristicStart->getData());
    delay(3);

    std::string rx_rct_str = pCharacteristicRct->getValue();
    if(rx_rct_str.length() > 0)
      rct_estimate = std::stof(rx_rct_str);
    
    std::string rx_rs_str  = pCharacteristicRs->getValue();
    if(rx_rs_str.length() > 0)
      rs_estimate  = std::stof(rx_rs_str);
  }while(!start_value || old_start_value == start_value || digitalRead(BUTTON));
  
  // int buttonStatus = digitalRead(BUTTON); 
  delay(10); // Additional Debounce delay - 10 ms 

  // if(buttonStatus == LOW)
  // {
  digitalWrite(LEDPIN, LOW);
  delay(1000);
  Serial.println("Button pressed - starting measurements!");
  blinkLED(3, 0);
    
  /* Main Testing Code - also used for current draw as a standard sweep measurement */
  /* Start, Stop, NumPoints, Vbias, Vzero, Rcal, gain array, gain size, Excitation Gain, DAC Gain*/
  demo.AD5940_TDD(100000, 0.15, 6, 0.0, 0.0, 9870, test, gainSize, 1, 1);
  demo.runSweep(numCycles, delaySecs); // Run the Sweep
  std::vector<float> resistors = demo.calculateResistors(rct_estimate, rs_estimate);
  Serial.print(resistors[0]);
  Serial.print(",");
  Serial.println(resistors[1]);
  demo.saveDataEIS("folder-name-here", "file-name-here");

    /* Current Draw Code - (no sweep but set for measurement)*/
//    demo.AD5940_TDD(startFreq, endFreq, 6, 0.0, 0.0, 9870, test, gainSize, 1, 1);
//    demo.configureFrequency(startFreq);

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