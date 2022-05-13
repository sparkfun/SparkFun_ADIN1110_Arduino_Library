//
#include "sfe_spe_advanced.h"
#include <Wire.h>
#include <SerLCD.h>
/* Extra 4 bytes for FCS and 2 bytes for the frame header */
#define MAX_FRAME_BUF_SIZE  (MAX_FRAME_SIZE + 4 + 2)

#define MAC_ADDR_0_0        (0x00)
#define MAC_ADDR_0_1        (0xE0)
#define MAC_ADDR_0_2        (0x22)
#define MAC_ADDR_0_3        (0xFE)
#define MAC_ADDR_0_4        (0xDA)
#define MAC_ADDR_0_5        (0xC9)

#define MAC_ADDR_1_0        (0x00)
#define MAC_ADDR_1_1        (0xE0)
#define MAC_ADDR_1_2        (0x22)
#define MAC_ADDR_1_3        (0xFE)
#define MAC_ADDR_1_4        (0xDA)
#define MAC_ADDR_1_5        (0xCA)

uint8_t macAddr[2][6] = {
    {MAC_ADDR_0_0, MAC_ADDR_0_1, MAC_ADDR_0_2, MAC_ADDR_0_3, MAC_ADDR_0_4, MAC_ADDR_0_5},
    {MAC_ADDR_1_0, MAC_ADDR_1_1, MAC_ADDR_1_2, MAC_ADDR_1_3, MAC_ADDR_1_4, MAC_ADDR_1_5},
};

sfe_spe_advanced adin1110;
SerLCD lcd;

/* Number of buffer descriptors to use for both Tx and Rx in this example */
#define BUFF_DESC_COUNT     (6)

HAL_ALIGNED_PRAGMA(4)
static uint8_t rxBuf[BUFF_DESC_COUNT][MAX_FRAME_BUF_SIZE] HAL_ALIGNED_ATTRIBUTE(4);

/* Example configuration */
uint8_t sample_data_num = 0;
uint8_t txBuffidx = 0;
adi_eth_BufDesc_t       rxBufDesc[BUFF_DESC_COUNT];

char display_text[100];
char display_updated = false;

static void rxCallback(void *pCBParam, uint32_t Event, void *pArg)
{
    adin1110_DeviceHandle_t hDevice = (adin1110_DeviceHandle_t)pCBParam;
    adi_eth_BufDesc_t       *pRxBufDesc;
    uint32_t                idx;

    pRxBufDesc = (adi_eth_BufDesc_t *)pArg;

//    Serial.print("Recieved: ");
//
//    Serial.printf((char *)&pRxBufDesc->pBuf[14]);
    memset(display_text, '\0', 100);
    strncpy(display_text, (char *)&pRxBufDesc->pBuf[14], 99);
    display_updated = true;

//    Serial.println();
    
    /* Since we're not doing anything with the Rx buffer in this example, */
    /* we are re-submitting it to the queue. */
    adin1110.submitRxBuffer(pRxBufDesc);
}

volatile adi_eth_LinkStatus_e    linkStatus;
void cbLinkChange(void *pCBParam, uint32_t Event, void *pArg)
{
    linkStatus = *(adi_eth_LinkStatus_e *)pArg;
    memset(display_text, '\0', 100);
    if(linkStatus ==ADI_ETH_LINK_STATUS_UP )
    {
//      lcd.print("Connected!\r\n");
        
        strncpy(display_text, "\r\nConnected", 99);
        
    }
    else
    {
//      lcd.clear();
//      lcd.print("Disconnected!\r\n");
        strncpy(display_text, "Disconnected", 99);
    }
    display_updated = true;
}

void setup() 
{
    adi_eth_Result_e        result;
    uint32_t                error;
    adin1110_DeviceStruct_t dev;
    adin1110_DeviceHandle_t hDevice = &dev;
    uint32_t                heartbeatCheckTime = 0;
    Wire.begin();
    Wire.setClock(100000);
    lcd.begin(Wire); //Set up the LCD for I2C communication
    lcd.clear(); //Clear the display - this moves the cursor to home position as well
    
    Serial.begin(115200);
    while (!Serial) {
      ; // wait for serial port to connect. Needed for native USB port only
    }
    /****** System Init *****/
        result = adin1110.begin();
    if(result != ADI_ETH_SUCCESS) Serial.println("No MACPHY device found");
    else Serial.println("Adin1110 found!");

    result = adin1110.addAddressFilter(&macAddr[0][0], NULL, 0);
    if(result != ADI_ETH_SUCCESS) Serial.println("adin1110_AddAddressFilter");

    result = adin1110.addAddressFilter(&macAddr[1][0], NULL, 0);
    if(result != ADI_ETH_SUCCESS) Serial.println("adin1110_AddAddressFilter");

    result = adin1110.syncConfig();
    if(result != ADI_ETH_SUCCESS) Serial.println("adin1110_SyncConfig");

    result = adin1110.registerCallback(cbLinkChange, ADI_MAC_EVT_LINK_CHANGE);
    if(result != ADI_ETH_SUCCESS) Serial.println("adin1110_RegisterCallback (ADI_MAC_EVT_LINK_CHANGE)");

    /* Prepare Tx/Rx buffers */
    for (uint32_t i = 0; i < BUFF_DESC_COUNT; i++)
    {
        //Submit All rx buffers
        rxBufDesc[i].pBuf = &rxBuf[i][0];
        rxBufDesc[i].bufSize = MAX_FRAME_BUF_SIZE;
        rxBufDesc[i].cbFunc = rxCallback;
        
        result = adin1110.submitRxBuffer(&rxBufDesc[i]);
    }

    result = adin1110.enable();
    if(result != ADI_ETH_SUCCESS) Serial.println("Device enable error");

    /* Wait for link to be established */
    lcd.print("Waiting for connection...");
    unsigned long prev = millis();
    unsigned long now;
    while (linkStatus != ADI_ETH_LINK_STATUS_UP)
    {
        now = millis();
        if( (now - prev) >= 1000)
        {
          prev = now;
          lcd.print(".");
        }
    }
}

void loop() {
     digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
     if(display_updated)
     {
        lcd.clear();
        int n = 0;
        int output_size = strnlen(display_text, MAX_FRAME_BUF_SIZE);
        const int chunk_size = 16;
        lcd.clear();
        while(output_size>chunk_size){
          lcd.write((uint8_t *)&display_text[n], chunk_size);
          output_size -= chunk_size;
          n += chunk_size;
        }
        lcd.write((uint8_t *)&display_text[n], output_size);
       display_updated = false;
     }
     delay(5);
}
