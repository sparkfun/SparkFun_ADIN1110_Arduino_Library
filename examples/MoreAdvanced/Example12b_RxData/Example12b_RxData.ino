/*
 *---------------------------------------------------------------------------
 *
 * Copyright (c) 2020, 2021 Analog Devices, Inc. All Rights Reserved.
 * This software is proprietary to Analog Devices, Inc.
 * and its licensors.By using this software you agree to the terms of the
 * associated Analog Devices Software License Agreement.
 *
 *---------------------------------------------------------------------------
 */
#include "sfe_spe_advanced.h"
/* Extra 4 bytes for FCS and 2 bytes for the frame header */
#define MAX_FRAME_BUF_SIZE  (MAX_FRAME_SIZE + 4 + 2)

#define FRAME_HEADER_SIZE 14

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

/* Number of buffer descriptors to use for both Tx and Rx in this example */
#define BUFF_DESC_COUNT     (6)

HAL_ALIGNED_PRAGMA(4)
static uint8_t rxBuf[BUFF_DESC_COUNT][MAX_FRAME_BUF_SIZE] HAL_ALIGNED_ATTRIBUTE(4);

/* Example configuration */
uint8_t sample_data_num = 0;
uint8_t txBuffidx = 0;
adi_eth_BufDesc_t       rxBufDesc[BUFF_DESC_COUNT];

#define PRINT_STRING

static void rxCallback(void *pCBParam, uint32_t Event, void *pArg)
{
    adin1110_DeviceHandle_t hDevice = (adin1110_DeviceHandle_t)pCBParam;
    adi_eth_BufDesc_t       *pRxBufDesc;
    uint32_t                idx;

    pRxBufDesc = (adi_eth_BufDesc_t *)pArg;

    Serial.print("Recieved: ");
    
    for(int i = 0; i < (pRxBufDesc->trxSize - FRAME_HEADER_SIZE); i++)
    {
      Serial.print(pRxBufDesc->pBuf[i+FRAME_HEADER_SIZE]);
      Serial.print(" ");
    }

    Serial.println();
    
    /* Since we're not doing anything with the Rx buffer in this example, */
    /* we are re-submitting it to the queue. */
    adin1110.submitRxBuffer(pRxBufDesc);
}

volatile adi_eth_LinkStatus_e    linkStatus;
void cbLinkChange(void *pCBParam, uint32_t Event, void *pArg)
{
    linkStatus = *(adi_eth_LinkStatus_e *)pArg;
    if(linkStatus ==ADI_ETH_LINK_STATUS_UP )
    {
      Serial.println("Connected!");
    }
    else
    {
      Serial.println("Disconnected!");
    }
}

void setup() 
{
    adi_eth_Result_e        result;
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
    Serial.print("Device configured, waiting for connection...");
    unsigned long prev = millis();
    unsigned long now;
    while (linkStatus != ADI_ETH_LINK_STATUS_UP)
    {
        now = millis();
        if( (now - prev) >= 1000)
        {
          prev = now;
          Serial.print(".");
        }
    }
}

void loop() {
     digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
     
     delay(1000);
}
