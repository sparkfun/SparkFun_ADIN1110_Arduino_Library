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
        Triple Axis Accelerometer KX132         https://www.sparkfun.com/products/17871
        Qwiic OLED Display (128 x 32)           https://www.sparkfun.com/products/17153
        Qwiic Cables                            https://www.sparkfun.com/products/15081

    Note: Either a KX134 or KX132 sensor can be used - both were used for this demo  


    OVERVIEW
        This application monitors the output of the attached accelerometers, and if the overall
        vibration magnatude exceeds a set limit, the attached pump run value is set to "ON", 
        otherwise, it's set to "OFF".

        The vibration limits for each pump were done via observation. It's possible to build in
        a calibration step to determine this in the future.

        The state of the pumps are displayed on the attached OLED screen, and also set 
        to the relay board via the SPE connection. All data is sent using a json format,
        and following the schema used by MachineChat (the used IoT server).

        NOTE: Sensor 2, the KX132, is using its alternative I2C address - the on-board jumper needs cutting. 

    Written by Kirk Benell @ SparkFun Electronics, May 2022

    SPE Library Repository:
        https://github.com/sparkfun/SparkFun_ADIN1110_Arduino_Library

    SparkFun code, firmware, and software is released under the MIT License(http://opensource.org/licenses/MIT).  

*/

#include <stdbool.h>
#include <math.h>
#include <ArduinoJson.h>

#include <SparkFun_Qwiic_OLED.h>
#include <res/qw_fnt_8x16.h>
#include <SparkFun_SinglePairEthernet.h>
#include <SparkFun_Qwiic_KX13X.h>

// Setup for SPE device - mac addresses for communication
byte deviceMAC[6] = {0x00, 0xE0, 0x22, 0xFE, 0xDA, 0xC9};
byte destinationMAC[6] = {0x00, 0xE0, 0x22, 0xFE, 0xDA, 0xCA};

// SPE board
SinglePairEthernet speDevice;

// Our sensor objects
QwiicKX134 kx134Sensor01;
QwiicKX132 kx132Sensor02;

// Our OLED display unit. 
QwiicNarrowOLED myOLED;

// sensor flags
bool bSensor01Connected = false;
bool bSensor02Connected = false;

// On/Off Threshold for the pumps -- determined by observation
#define PUMP_1_ON_THRESHOLD 0.025
#define PUMP_2_ON_THRESHOLD 0.05

// Local Machine Chat information
const char * SensorID = "BasementPumps";
const char * machineChatURL = "/v1/data/mc";

#define SAMPLE_PERIOD_MS  2000

static uint32_t lastSampleTicks = 0;

// for managing the data from the pumps
bool pump01LastDetect = false;
bool pump02LastDetect = false;

bool pump01State = false;
bool pump02State = false;

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
        Serial.println("Unable to communicate with Sensor 01");
    else
        Serial.println("Sensor 01 Connected");


    // Sensor two -- not the use of it's ALT Address 
    bSensor02Connected = kx132Sensor02.begin(KX13X_ALT_ADDRESS); //Begin communication over I2C
    if (!bSensor02Connected)
        Serial.println("Unable to communicate with Sensor 02");
    else
        Serial.println("Sensor 02 Connected");

    // No Sensors, no dice for this work
    if (!bSensor02Connected && !bSensor01Connected){
        // No sensors connected

        Serial.println("No Sensors connected - halting");
        while(1); 
    }

    // Initialize the connected sensors
    if (bSensor01Connected && !kx134Sensor01.initialize(DEFAULT_SETTINGS))
    {
        Serial.println("Unable to initialize Sensor 01");
        bSensor01Connected = false;
    }

    // Init the sensors
    if (bSensor02Connected && !kx132Sensor02.initialize(DEFAULT_SETTINGS))
    {
        Serial.println("Unable to initialize Sensor 02");
        bSensor02Connected = false;
    }

    // Set some parameters on Sensor 02  
    kx132Sensor02.setRange(KX132_RANGE8G);
    kx132Sensor02.setOutputDataRate(0b0111);

    if (!bSensor02Connected && !bSensor01Connected){
        // No sensors connected

        Serial.println("No Sensors connected - halting");
        while(1); 
    }

    // Initalize the OLED device and related graphics system
    if (myOLED.begin() == false)
    {
        Serial.println("OLED begin failed. Freezing...");
        while (true)
            ;
    }
    myOLED.setFont(QW_FONT_8X16);

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

///////////////////////////////////////////////////////////////////////////////////////////
// takeDataSample()
//
// take a data sample, return a string of the results
//
// Returns true on success, false of fialure.
static bool takeDataSample(String& pump01Results, String &pump02Results){

    float xzMagSensor02 = 0;
    float xzMagSensor01 = 0;

    outputData pump01Data;
    outputData pump02Data;    
    bool pump01IsOn;
    bool pump02IsOn;

    if (bSensor01Connected)
    {
        pump01Data = kx134Sensor01.getAccelData(); 
    
        // Normalize the X-Z data

        xzMagSensor01 = sqrtf(pump01Data.xData * pump01Data.xData + pump01Data.zData * pump01Data.zData);

        pump01IsOn = (xzMagSensor01 > 0.03 || xzMagSensor01 < 0.0200 );

    }

    if (bSensor02Connected)
    {
        pump02Data = kx132Sensor02.getAccelData(); 
    
        
        // Normalize the X-Z data

        xzMagSensor02 = sqrtf(pump02Data.xData * pump02Data.xData + pump02Data.zData * pump02Data.zData);


        pump02IsOn  = xzMagSensor02 > 0.0420 || xzMagSensor02 < 0.035 ;
    
    }

    // If the last detect and this detect are the same - change state. Helps deal with single sample outliners
    if(pump01LastDetect == pump01IsOn)
        pump01State = pump01IsOn;

    pump01LastDetect  = pump01IsOn;

    if(pump02LastDetect == pump02IsOn)
        pump02State = pump02IsOn;

    pump02LastDetect  = pump02IsOn;

    // Output value to Screen
    myOLED.erase();
    myOLED.setCursor(0,0);
    myOLED.print("Pump 01:");
    //myOLED.print(xzMag, 4);
    myOLED.println( pump01State ? "-ON-" : "<OFF>");

    myOLED.print("Pump 02:");
    //myOLED.print(xzMag, 4);
    myOLED.print( pump02State ? "-ON-" : "<OFF>");

    myOLED.display();

    // Package up the values in json doc -- follow schema outlined by machine chat

    StaticJsonDocument<256> jsonDoc;

    JsonObject jsonContext = jsonDoc.createNestedObject("context");

    jsonContext["target_id"] = String("Pump01");
    JsonObject jsonData = jsonDoc.createNestedObject("data");
    jsonData["X"] = pump01Data.xData;
    jsonData["Y"] = pump01Data.yData;
    jsonData["Z"] = pump01Data.zData;        
    jsonData["On"] = (int)pump01State;

    serializeJson(jsonDoc, pump01Results);

   // Serial.println(pump01Results);

    jsonDoc.clear();
    jsonContext = jsonDoc.createNestedObject("context");
    jsonContext["target_id"] = String("Pump02");
    jsonData = jsonDoc.createNestedObject("data");
    jsonData["X"] = pump02Data.xData;
    jsonData["Y"] = pump02Data.yData;
    jsonData["Z"] = pump02Data.zData;        
    jsonData["On"] = (int)pump02State;

    // Serialize the json document to a string.
    serializeJson(jsonDoc, pump02Results);
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
        String pump01Results, pump02Results; 
        if(takeDataSample(pump01Results, pump02Results))
        {
            // Send  the data to the connected SPE device

            speDevice.sendData((unsigned char*)pump01Results.c_str(), pump01Results.length(), destinationMAC);
            delay(300);
            speDevice.sendData((unsigned char*)pump02Results.c_str(), pump02Results.length(), destinationMAC);            

        }
        lastSampleTicks = millis();
    }else
        delay(500);
}
