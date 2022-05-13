#include "SparkFun_SinglePairEthernet.h"
#include <Arduino_JSON.h>
#include <SerLCD.h>

SinglePairEthernet adin1110;
SerLCD lcd;

const int MAX_CHARS_DISPLAY = 4*20;
//C string to hold entire display text + a Null terminator
char display_text[MAX_CHARS_DISPLAY+1];
char display_updated = false;
unsigned long lastBlink = 0;

byte deviceMAC[6] = {0x00, 0xE0, 0x22, 0xFE, 0xDA, 0xCA};

static void rxCallback(byte * data, int dataLen, byte * senderMAC)
{
    JSONVar BMEData = JSON.parse((char *)data);
    memset(display_text, '\0', MAX_CHARS_DISPLAY+1);
    sprintf(display_text, "Humidity: %d\r\nTemperature: %d\r\nPressure: %d\r\nAltitide %d\r\n",
      (int)BMEData["humidity"], (int)BMEData["temp"], (int)BMEData["pressure"], (int)BMEData["alt"]);
    display_updated = true;
}

void linkCallback(bool linkStatus)
{
    memset(display_text, '\0', MAX_CHARS_DISPLAY+1);
    strncpy(display_text, ( linkStatus ) ? "\r\nConnected" : "Disconnected", MAX_CHARS_DISPLAY);
    display_updated = true;
}

void setup() 
{
    Wire.begin();
    Wire.setClock(100000);
    lcd.begin(Wire); //Set up the LCD for I2C communication
    lcd.clear(); //Clear the display - this moves the cursor to home position as well
    
    Serial.begin(115200);
    while(!Serial);

    Serial.println("Single Pair Ethernet - Example 3b Recieve String from 3a and Display on SerLCD");
    /* Start up adin1110 */
    if(!adin1110.begin(deviceMAC)) 
    {
      Serial.print("Failed to connect to ADIN1110 MACPHY. Make sure board is connected and pins are defined for target.");
      while(1); //If we can't connect just stop here     
    }
    Serial.println("Connected to ADIN1110 MACPHY");

    /* Set up callback, to control what we do when data is recieved and when link changed*/
    adin1110.setRxCallback(rxCallback);
    adin1110.setLinkCallback(linkCallback);

    /* Wait for link to be established */
    lcd.print("Waiting for\nconnection...");
    while (adin1110.getLinkStatus() != true);
}

void loop() {
    unsigned long now = millis();
    if(now - lastBlink >= 1000)
    {
        digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
    }     
    if(display_updated)
    {
        lcd.clear();
        int n = 0;
        int output_size = strlen(display_text);
        const int chunk_size = 16;
        lcd.clear();
        while(output_size>chunk_size){
            lcd.write((byte *)&display_text[n], chunk_size);
            output_size -= chunk_size;
            n += chunk_size;
        }
        lcd.write((byte *)&display_text[n], output_size);
        display_updated = false;
    }
    delay(5);
}