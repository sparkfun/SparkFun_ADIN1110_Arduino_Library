#include "Sparkfun_SinglePairEth.h"

SinglePairEth adin1110;

unsigned long lastBlink = 0;

void setup() 
{
    adi_eth_Result_e        result;
    Serial.begin(115200);
    while (!Serial);

    /* Start up adin1110 */
    result = adin1110.begin();
    while (result != ADI_ETH_SUCCESS) 
    {
      Serial.print("Failed to connect to ADIN1110 MACPHY. Error Code: ");
      Serial.print(result);
      while(1);      
    }
    
    Serial.println("Connected to ADIN1110 MACPHY");

    /* Wait for link to be established */
    Serial.println("Device Configured, waiting for connection...");
    while (adin1110.linkStatus != ADI_ETH_LINK_STATUS_UP);
}

void loop() {
    if(adin1110.getRxAvailable())
    {
      uint8_t tempBuffer[1000];
      uint16_t length = adin1110.getRxData(tempBuffer, 1000);
      adin1110.sendData(tempBuffer, length);
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
