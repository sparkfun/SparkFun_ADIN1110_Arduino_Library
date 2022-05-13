#include "SparkFun_SinglePairEthernet.h"

SinglePairEthernet adin1110;

int msg = 0;
const int NUM_MSGS = 8;
const int MAX_MSG_SIZE = 200;

byte deviceMAC[6] = {0x00, 0xE0, 0x22, 0xFE, 0xDA, 0xC9};
byte destinationMAC[6] = {0x00, 0xE0, 0x22, 0xFE, 0xDA, 0xCA};

char outputString[NUM_MSGS][MAX_MSG_SIZE] = {
    "Adin1110 Example 1a",
    "If this sketch is connected to another devices running example 1b, these messages will be echoed back",
    "User can define their own behavior for when data is recieved by defining their own rxCallback",
    "The conterpart to this example demonstrates how to access recieved data if using callback is not desired",
    "This example uses Serial.println in the callback, this is may not be best practice since it can happen in an interrupt",
    "Basic functionality of sending and recieving data is provided by the SinglePairEthernet class",
    "Messages are copied to memory internal to the created object",
    "If more performance is required, or you would like more control over memory, try the sfe_spe_advanced class"
};

static void rxCallback(byte * data, int dataLen, byte * senderMac)
{
    Serial.print("Recieved:\t");
    Serial.println((char *)data); //This is ok since we know they are all null terminated strings
    if(!adin1110.indenticalMacs(senderMac, destinationMAC))
    {
        Serial.print("From an unknown source: ");
        for(int i = 0; i < 6; i++)
        {
          Serial.print(senderMac[i]);
          Serial.print(" ");    
        }
        
    }
    Serial.println();
}

void setup() 
{
    Serial.begin(115200);
    while(!Serial);

    Serial.println("Single Pair Ethernet - Example 1a Basic Send and Recieve");
    /* Start up adin1110 */
    if (!adin1110.begin(deviceMAC)) 
    {
      Serial.print("Failed to connect to ADIN1110 MACPHY. Make sure board is connected and pins are defined for target.");
      while(1); //If we can't connect just stop here      
    }
    Serial.println("Connected to ADIN1110 MACPHY");

    /* Set up callback, to control what we do when data is recieved */
    adin1110.setRxCallback(rxCallback);

    /* Wait for link to be established */
    Serial.println("Device Configured, waiting for connection...");
    while (adin1110.getLinkStatus() != true);
}

void loop() {
    /* If we are still connected, send a message */
    if(adin1110.getLinkStatus())
    {      
        Serial.print("Sending:\t");
        Serial.println(outputString[msg]); //This is ok since we know they are all null terminated strings
        
        adin1110.sendData((byte *)outputString[msg], sizeof(outputString[msg]), destinationMAC);
        
        msg++;
        if(msg >= NUM_MSGS)
        {
          msg = 0;
        }
    }
    else
    {
        Serial.println("Waiting for link to resume sending");
    }

    digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));    
    delay(5000);
}