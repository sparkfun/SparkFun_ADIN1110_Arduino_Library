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

#include <SparkFun_SinglePairEthernet.h>
#include <Ethernet.h>
#include <stdbool.h>

// Our SPE Device object
SinglePairEthernet speDevice;

// Define a MAC address for the SPE connection
byte speMAC[6] = {0x00, 0xE0, 0x22, 0xFE, 0xDA, 0xCA};

////////////////////////////////////////////////////////
// Ethernet Setup
// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
byte ethernetMac[] = {0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED};

// Fallbacks - Set the static IP address to use if the DHCP fails to assign
IPAddress ip(192, 168, 0, 177);
IPAddress myDns(192, 168, 0, 1);


IPAddress machineServerIP;
EthernetClient ethernetClient;

/////////////////////////////////////////
// Our IoT Server info

const int serverPort = 8100;
const char *serverAddress = "10.7.2.21";
const char *machineChatURL = "/v1/data/mc";

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
// setupEthernet()
//
// Function to setup the Ethernet device - returns true on success, false on error
//
// Note: The Ethernet Function board should be in Slot 1
//
bool setupEthernet(void)
{

    Ethernet.init(9); // rp2040 = ethernet in slot 1

    // start the Ethernet connection:

    if (Ethernet.begin(ethernetMac) == 0)
    {
        // Check for Ethernet hardware present
        if (Ethernet.hardwareStatus() == EthernetNoHardware)
        {
            Serial.println("Ethernet board was not found.  Sorry, can't run without hardware. :(");
            return false;
        }
        Ethernet.begin(ethernetMac, ip, myDns);
    }
    else
    {
        Serial.print("Ethernet - DHCP assigned IP ");
        Serial.println(Ethernet.localIP());
    }

    // setup our server address
    if (!machineServerIP.fromString(serverAddress))
    {
        Serial.println("ERROR - Invalid Server address.");
        return false;
    }

    // If we are here, we're ready to rock!
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
bool sendDataToServer(char *data)
{

    // Connect to the machine chat server
    if (!ethernetClient.connect(machineServerIP, serverPort))
    {
        Serial.println("Error - Unable to connect to the IoT server");
        return false;
    }

    // send our data to the server via HTTP, with a JSON payload. 

    char charBuffer[128];

    // Write out our HTTP headers
    snprintf(charBuffer, sizeof(charBuffer), "POST %s HTTP/1.1", machineChatURL);
    ethernetClient.println(charBuffer);

    snprintf(charBuffer, sizeof(charBuffer), "Host: %s:%d", serverAddress, serverPort);
    ethernetClient.println(charBuffer);

    ethernetClient.println(F("Content-Type: application/json"));
    ethernetClient.println(F("Connection: close"));
    ethernetClient.print(F("Content-Length: "));
    ethernetClient.println(strlen(data));
    ethernetClient.println();
    ethernetClient.print(data);
    ethernetClient.println();
    ethernetClient.flush();

    delay(200);  // let the device send the data

    ethernetClient.stop();  // This transaction is done

    return true; // SUCCESS!
}
///////////////////////////////////////////////////////////////////////////////////////////
// setup()
//

void setup()
{

    Serial.begin(115200);

    // Note: the rp2040 doesn't like this next statement 
    //while (!Serial)
        //;

    // setup SPE
    if (!setupSPE())
    {
        Serial.println("Unable to setup the SPE connection. Halting.");
        while (true)
            ;
    }

    if (!setupEthernet())
    {
        Serial.println("Unable to setup the Ethernet connection. Halting.");
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

    if( Ethernet.linkStatus() != LinkOFF)
    {
        // Loop over the recieved data list. Non-null entry indicates new data.
        for(int i=0; i < DATA_BUFFER_LEN; i++ ){

            if(dataResults[i][0])  // Data?!
            {
                if (!sendDataToServer(dataResults[i]))
                    Serial.println("Error - Failed to send data to IoT server");                

                // clear out the current entry. 
                memset(dataResults[i], '\0', DATA_BUFFER_LEN);
            }

        }

    }
    delay(500);
}
