/*

    SparkFun Sump Pump Demo - spe_relay.ino

    This firmware is part of the SparkFun Sump Pump Single Pair Ethernet demo used
    for the June 2022 Webinar.

    This firmware is for the "relay" board used in the demo. This board consisted 
    of the following components

        SparkFun MicroMod Main Board Double     https://www.sparkfun.com/products/18576
        SparkFun MicroMod rp2030 Processor      https://www.sparkfun.com/products/17720
        SparkFun MicroMod Ethernet Function     https://www.sparkfun.com/products/18708
        SparkFun MicroMod SPE Function          https://www.sparkfun.com/products/19038
        Single Pair Ethernet Cable 0.5 m        https://www.sparkfun.com/products/19312

    The code assumes MM SPE Function board is in slot 0, and the Ethernet Function board
    is in slot 1.

    OVERVIEW
        This application recieves a string from the sensor board, and relays this board to 
        the IoT data plotting system (MachineChat) on the local network using the connected
        ethernet connection. 

    PARAMETERS
        Below parameters for the server location and URL used are defined. Change for the
        local installation as needed.

    Written by Kirk Benell @ SparkFun Electronics, May 2022

    SPE Library Repository:
        https://github.com/sparkfun/SparkFun_ADIN1110_Arduino_Library

    SparkFun code, firmware, and software is released under the MIT License(http://opensource.org/licenses/MIT).  

*/

#include <Arduino.h>
#include <Wire.h>
#include <SparkFun_SinglePairEthernet.h>
#include <stdbool.h>
#include <ArduinoJson.h>

// Our SPE Device object
SinglePairEthernet speDevice;

// Define a MAC address for the SPE connection
byte speMAC[6] = {0x00, 0xE0, 0x22, 0xFE, 0xDA, 0xCA};



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

////////////////////////////////////
// Result data variables
#define MAX_RESULTS_SIZE 256
#define DATA_BUFFER_LEN 3

// Note dataResults is a 2d array - it's a rotating list of recieved strings 
char dataResults[DATA_BUFFER_LEN][MAX_RESULTS_SIZE] = {'\0'};
volatile short nextData = 0; 

///////////////////////////////////////////////////////////////////////////////////////////
// rxCallback()
//
// SPE data received callback function.
// 
// This function registered with the SPE device and is called when new data is available.
//
// The data - which is just a char - is packed into the next open entry of dataResults
//
// NOTE - this is a local function - aka static 
//
///////////////////////////////////////////////////////////////////////////////////////////

static void rxCallback(byte *data, int lenData, byte *senderMac)
{
    
    if(lenData < 1 || lenData >= MAX_RESULTS_SIZE)
        return;

    // We have data - yay - the data is just a char array.
    memcpy(dataResults[nextData], (char*)data, lenData);
    dataResults[nextData][lenData]='\0'; // make a c string
    nextData = (nextData+1) % DATA_BUFFER_LEN;   // NEXT open entry in the data array

}

///////////////////////////////////////////////////////////////////////////////////////////
// setupSPE()
//
// Function to setup the SPE device - returns true on success, false on error
//
// Note: The SPE Function board should be in Slot 0

bool setupSPE(void)
{
    // init the SPE device with the assigned mac
    if (!speDevice.begin(speMAC))
        return false;

    // register our received data callback function
    speDevice.setRxCallback(rxCallback);

    return true;
}

///////////////////////////////////////////////////////////////////////////////////////////
// sendDatToServer()
//
// Function called to send a string to the MachineChat server via an HTTP call over 
// the Ethernet Function board connection.
//
// Returns true on success, false  on error
//
// Parameter:
//      data       c-string     - the data sent to the IOT server. It a JSON format.
bool updateDisplay(char *data)
{

    StaticJsonDocument<256> jsonDoc;

    DeserializationError error = deserializeJson(jsonDoc, data);

    if(error)
        return false;

    bool pump01State = jsonDoc["On"] ? true : false;

    Serial.println(pump01State);
    writeToLCD(pump01State ? "Vibration" : "No Vibration");
    if(pump01State)
        setLCDColor(255, 0 ,0);
    else
        setLCDColor(0, 255, 0);

    return true; // SUCCESS!
}
///////////////////////////////////////////////////////////////////////////////////////////
// setup()
//

void setup()
{

    Serial.begin(115200);

    // Note: the rp2040 doesn't like this next statement 
    while (!Serial)
        ;

    // Setup our sensors 
    Wire.begin();

    // setup SPE
    if (!setupSPE())
    {
        Serial.println("Unable to setup the SPE connection. Halting.");
        while (true)
            ;
    }

}
///////////////////////////////////////////////////////////////////////////////////////////
// loop()
//

void loop()
{
    // if the Ethernet device is connected to the network, check to see if any
    // data is available - contained in the dataResults[] array.
    //
    // Note: Data is added to dataReslts() in the SPE Rx callback 
    //
// are we connected to the network
    if (!speDevice.getLinkStatus())
    {
        Serial.println("Warning - No SPE connection.");
        delay(2000); // take a little nap
        return;
    }
    // Loop over the recieved data list. Non-null entry indicates new data.
    for(int i=0; i < DATA_BUFFER_LEN; i++ ){

        if(dataResults[i][0])  // Data?!
        {
            if (!updateDisplay(dataResults[i]))
                Serial.println("Error - Failed to send data to IoT server");                

            // clear out the current entry. 
            memset(dataResults[i], '\0', DATA_BUFFER_LEN);
        }

    }
    delay(200);
}
