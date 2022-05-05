#include "Sparkfun_SinglePairEth.h"

SinglePairEth adin1110;

int msg = 0;
const int kNumMsgs = 8;
const int kMaxMsgSize = 200;
char outputString[kNumMsgs][kMaxMsgSize] = {
  "Adin1110 Example 1a",
  "If this sketch is connected to another devices running example 1b, these messages will be echoed back",
  "User can define their own behavior for when data is recieved by defining their own rxCallback",
  "The conterpart to this example demonstrates how to access recieved data if using callback is not desired",
  "This example uses Serial.println in the callback, this is may not be best practice since it can happen in an interrupt",
  "Basic functionality of sending and recieving data is provided by the SinglePairEth class",
  "Messages are copied to memory internal to the created object",
  "If more performance is required, or you would like more control over memory, try the SinglePairEth_Raw class"
};


static void rxCallback(uint8_t * data, uint16_t dataLen)
{
    Serial.print("Recieved:\t");
    Serial.println((char *)data); //This is ok since we know they are all null terminated strings
    Serial.println();
}

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

    /* Set up callback, to control what we do when data is recieved */
    adin1110.setRxCallback(rxCallback);

    /* Wait for link to be established */
    Serial.println("Device Configured, waiting for connection...");
    while (adin1110.linkStatus != ADI_ETH_LINK_STATUS_UP);
}

void loop() {
    /* If we are still connected, send a message */
    if(adin1110.linkStatus == ADI_ETH_LINK_STATUS_UP)
    {      
        Serial.print("Sending:\t");
        Serial.println(outputString[msg]); //This is ok since we know they are all null terminated strings
        
        adin1110.sendData((uint8_t *)outputString[msg], sizeof(outputString[msg]));
        
        msg++;
        if(msg >= kNumMsgs)
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
