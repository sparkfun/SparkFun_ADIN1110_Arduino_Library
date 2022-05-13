#include "SparkFun_SinglePairEthernet.h"

SinglePairEthernet adin1110;

unsigned long lastBlink = 0;

byte deviceMAC[6] = {0x00, 0xE0, 0x22, 0xFE, 0xDA, 0xCA};
byte destinationMAC[6] = {0x00, 0xE0, 0x22, 0xFE, 0xDA, 0xC9};

void setup() 
{
    Serial.begin(115200);
    while(!Serial);

    Serial.println("Single Pair Ethernet - Example 1b Basic Echo");
    /* Start up adin1110 */
    if (!adin1110.begin(deviceMAC)) 
    {
      Serial.print("Failed to connect to ADIN1110 MACPHY. Make sure board is connected and pins are defined for target.");
      while(1); //If we can't connect just stop here  
    }
    
    Serial.println("Connected to ADIN1110 MACPHY");

    /* Wait for link to be established */
    Serial.println("Device Configured, waiting for connection...");
    while (adin1110.getLinkStatus() != true);
}

void loop() {
    if(adin1110.getRxAvailable()) 
    {
        byte tempBuffer[1000];
        byte senderMAC[6];
        int length = adin1110.getRxData(tempBuffer, 1000, senderMAC);
        adin1110.sendData(tempBuffer, length, senderMAC);
        Serial.print("Echoed:\t");
        Serial.println((char *)tempBuffer);
    }
    
    unsigned long now = millis();
    if(now - lastBlink >= 1000)
    {
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN)); 
        lastBlink = now;
    }
       
}