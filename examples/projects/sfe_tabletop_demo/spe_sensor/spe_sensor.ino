/*

    SparkFun Sump Pump Demo - spe_relay.ino

    This firmware is part of the SparkFun Sump Pump Single Pair Ethernet demo used
    for the June 2022 Webinar.

    This firmware is for the "sensor" board used in the demo. This board consisted 
    of the following components

        SparkFun MicroMod Main Board Single     https://www.sparkfun.com/products/18575
        SparkFun MicroMod Artemis Processor     https://www.sparkfun.com/products/16401
        SparkFun MicroMod SPE Function          https://www.sparkfun.com/products/19038
        Single Pair Ethernet Cable 0.5 m        https://www.sparkfun.com/products/19312
        Triple Axis Accelerometer KX134         https://www.sparkfun.com/products/17589
        Qwiic OLED Display (128 x 32)           https://www.sparkfun.com/products/17153
        Qwiic Cables                            https://www.sparkfun.com/products/15081

    OVERVIEW
        This application monitors the output of the attached accelerometers, and if the overall
        vibration magnatude exceeds a set limit, the attached pump run value is set to "ON", 
        otherwise, it's set to "OFF".

        The vibration limits for each pump were done via observation. It's possible to build in
        a calibration step to determine this in the future.

        The state of the pumps are displayed on the attached OLED screen, and also set 
        to the relay board via the SPE connection. All data is sent using a json format,
        and following the schema used by MachineChat (the used IoT server).

    Written by Kirk Benell @ SparkFun Electronics, May 2022

    SPE Library Repository:
        https://github.com/sparkfun/SparkFun_ADIN1110_Arduino_Library

    SparkFun code, firmware, and software is released under the MIT License(http://opensource.org/licenses/MIT).  

*/

#include <stdbool.h>
#include <math.h>
#include <ArduinoJson.h>


#include <SparkFun_SinglePairEthernet.h>
#include <SparkFun_Qwiic_KX13X.h>

// Setup for SPE device - mac addresses for communication
byte deviceMAC[6] = {0x00, 0xE0, 0x22, 0xFE, 0xDA, 0xC9};
byte destinationMAC[6] = {0x00, 0xE0, 0x22, 0xFE, 0xDA, 0xCA};

// SPE board
SinglePairEthernet speDevice;

// Our sensor objects
QwiicKX134 kx134Sensor01;

// sensor flags
bool bSensor01Connected = false;

// On/Off Threshold for the pumps -- determined by observation
#define PUMP_1_ON_THRESHOLD 0.025

// Local Machine Chat information
const char * SensorID = "BasementPumps";
const char * machineChatURL = "/v1/data/mc";

#define SAMPLE_PERIOD_MS  1000

static uint32_t lastSampleTicks = 0;

// for managing the data from the pumps
bool pump01LastDetect = false;

bool pump01State = false;

float lastSample[3]={0.0};

#define LCD_ADDRESS 0x72

///////////////////////////////////////////////////////////////////////////////////////////
// Write to LCD

void writeToLCD(const char *text){

  Wire.beginTransmission(LCD_ADDRESS); // transmit to device #1

  Wire.write('|'); //Put LCD into setting mode
  Wire.write('-'); //Send clear display command

  Wire.print(text);

  Wire.endTransmission(); //Stop I2C transmission

}
void setLCDColor(uint8_t r, uint8_t g, uint8_t b){

  Wire.beginTransmission(LCD_ADDRESS); // transmit to device #1
  //Wire.write('|'); //Put LCD into setting mode
  //Wire.write('-'); //Send clear display command  
  Wire.write('|'); //Put LCD into setting mode
  Wire.write('+'); //Send the Set RGB command
  Wire.write(r); //Send the red value
  Wire.write(g); //Send the green value
  Wire.write(b); //Send the blue value

  Wire.endTransmission(); //Stop I2C transmission

}
///////////////////////////////////////////////////////////////////////////////////////////
// setup()

void setup() 
{
   
    Serial.begin(115200);
    while (!Serial) {
      ; // wait for serial port to connect. Needed for native USB port only
    }

    // Setup our sensors 
    Wire.begin();
  
    // Sensor one
    bSensor01Connected = kx134Sensor01.begin();
    if (!bSensor01Connected)
    {
        Serial.println("Unable to communicate with Sensor 01. Halting");
        while(1);
    }
    else
        Serial.println("Sensor 01 Connected");


    // Initialize the connected sensors
    if (bSensor01Connected && !kx134Sensor01.initialize(DEFAULT_SETTINGS))
    {
        Serial.println("Unable to initialize Sensor 01 - halting");
        while(1);
    }

    // Initialize our network connection

    if(!speDevice.begin(deviceMAC))
    {
        Serial.println("Error - Failed to initialize the SPE Device. Halting.");
        while(1);
    }
    // Wait for link to be established 
    Serial.print("Device Configured, waiting for connection...");

    do
    {
        delay(100);
    } while ( !speDevice.getLinkStatus()); // while not connected ....
    Serial.println("Connected.");

    // adjust our tick counter so first loop a data sample is taken
    lastSampleTicks = millis() - SAMPLE_PERIOD_MS; // force a sample on first loop
}

#define VIBRATION_POINT .2

///////////////////////////////////////////////////////////////////////////////////////////
// takeDataSample()
//
// take a data sample, return a string of the results
//
// Returns true on success, false of fialure.
static bool takeDataSample(String& pump01Results){

    float accMag = 0;

    outputData pump01Data;

    bool pump01IsOn;
    float dataDeltas[3];

    if (bSensor01Connected)
    {
        pump01Data = kx134Sensor01.getAccelData(); 
    
        // change in accel from last sample?
        dataDeltas[0] = pump01Data.xData - lastSample[0];
        dataDeltas[1] = pump01Data.yData - lastSample[1];
        dataDeltas[2] = pump01Data.zData - lastSample[2];                

        // stash this reading
        lastSample[0] = pump01Data.xData;
        lastSample[1] = pump01Data.yData;
        lastSample[2] = pump01Data.zData;

        // size of delta
        accMag =  sqrtf(dataDeltas[0] * dataDeltas[0] + dataDeltas[1] * dataDeltas[1] + dataDeltas[2] * dataDeltas[2] );

        Serial.println(accMag);
        pump01IsOn = (accMag > VIBRATION_POINT);

    }

    Serial.println(pump01IsOn);
    // If the last detect and this detect are the same - change state. Helps deal with single sample outliners
    //if(pump01LastDetect == pump01IsOn)
    pump01State = pump01IsOn;

    //pump01LastDetect  = pump01IsOn;

    writeToLCD(pump01State ? "Vibration" : "No Vibration");
    if(pump01State)
        setLCDColor(255, 0 ,0);
    else
        setLCDColor(0, 255, 0);
   // myOLED.display();

    // Package up the values in json doc -- follow schema outlined by machine chat

    StaticJsonDocument<256> jsonDoc;

    jsonDoc["X"] = pump01Data.xData;
    jsonDoc["Y"] = pump01Data.yData;
    jsonDoc["Z"] = pump01Data.zData;        
    jsonDoc["On"] = (int)pump01State;

    serializeJson(jsonDoc, pump01Results);

    //Serial.println(pump02Results);    
    return true;

}
///////////////////////////////////////////////////////////////////////////////////////////
// loop()

void loop() {

    
    // are we connected to the network
    if (!speDevice.getLinkStatus())
    {
        Serial.println("Warning - No network connection.");
        delay(SAMPLE_PERIOD_MS*4); // take a little nap
        return;
    }

    // Take a measurement? 
    if(millis() - lastSampleTicks > SAMPLE_PERIOD_MS)
    {
        String pump01Results; 
        if(takeDataSample(pump01Results))
        {
            // Send  the data to the connected SPE device

            speDevice.sendData((unsigned char*)pump01Results.c_str(), pump01Results.length(), destinationMAC);
        }
        lastSampleTicks = millis();
    }else
        delay(200);
}
