#include "SparkFun_SinglePairEthernet.h"

SinglePairEthernet adin1110;

byte deviceMAC[6] = {0x00, 0xE0, 0x22, 0xFE, 0xDA, 0xC9};

//Attached in set-up to be called when the link status changes, parameter is current link status
void linkCallback(bool linkStatus)
{
    digitalWrite(LED_BUILTIN, linkStatus);
}

void setup() 
{
    Serial.begin(115200);
    while(!Serial);

    Serial.println("Single Pair Ethernet - Example 2 Link Status");
    /* Start up adin1110 */
    if (!adin1110.begin(deviceMAC)) 
    {
      Serial.print("Failed to connect to ADIN1110 MACPHY. Make sure board is connected and pins are defined for target.");
      while(1); //If we can't connect just stop here  
    }
    
    Serial.println("Connected to ADIN1110 MACPHY");

    /* Set up callback */
    adin1110.setLinkCallback(linkCallback);
}

void loop() {
  /* The purpose of this example is to show how to use the link callback function.
      Connect and disconnect to a board running example 01b, or another board running this example
      to see the onboard led change when the link status changes.
      No logic needed here.
  */
  /* If not using a callback you could need to do the following to get the same functionality*/
  /*
  static bool linkStatus = false;
  bool currentStatus = adin1110.getLinkStatus();
  if(linkStatus != currentStatus)
  {
    linkStatus = currentStatus;
    digitalWrite(LED_BUILTIN, linkStatus);
  }
  */
}