#include "Sparkfun_SinglePairEth.h"

SinglePairEth adin1110;

//Attached in set-up to be called when the link status changes, parameter is current link status
void linkCallback(adi_eth_LinkStatus_e linkStatus)
{
  if(linkStatus == ADI_ETH_LINK_STATUS_UP)
  {
    digitalWrite(LED_BUILTIN, HIGH);
  }
  else
  {
    digitalWrite(LED_BUILTIN, LOW);
  }
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

    /* Set up callback */
    adin1110.setLinkCallback(linkCallback);

}

void loop() {
  /* The purpose of this example is to show how to use the link callback function.
      Connect and disconnect to a board running example 01b, or another board running this example
      to see the onboard led change when the link status changes.
      No logic needed here.
  */
}