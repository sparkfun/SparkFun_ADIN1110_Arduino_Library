
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
#define MIN_PAYLOAD_SIZE    (46)

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

#define FRAME_HEADER_SIZE 14

uint8_t frame_header[FRAME_HEADER_SIZE] =
{
      MAC_ADDR_0_0, MAC_ADDR_0_1, MAC_ADDR_0_2, MAC_ADDR_0_3, MAC_ADDR_0_4, MAC_ADDR_0_5,
      MAC_ADDR_1_0, MAC_ADDR_1_1, MAC_ADDR_1_2, MAC_ADDR_1_3, MAC_ADDR_1_4, MAC_ADDR_1_5,
      0x08, 0x00,
};

sfe_spe_advanced adin1110;
uint8_t outputValue = 0; //just a number that we are going to increment that will rollover every 256 bytes


/* Number of buffer descriptors to use for both Tx and Rx in this example */
#define BUFF_DESC_COUNT     (6)
#define FRAME_SIZE          (1518)

HAL_ALIGNED_PRAGMA(4)
static uint8_t rxBuf[BUFF_DESC_COUNT][MAX_FRAME_BUF_SIZE] HAL_ALIGNED_ATTRIBUTE(4);

HAL_ALIGNED_PRAGMA(4)
static uint8_t txBuf[BUFF_DESC_COUNT][MAX_FRAME_BUF_SIZE] HAL_ALIGNED_ATTRIBUTE(4);
bool txBufAvailable[BUFF_DESC_COUNT];

/* Example configuration */
float reported_humidity;
float reported_pressure;
float reported_alt;
float reported_temp;
unsigned long last_report;
unsigned long last_toggle;
uint8_t sample_data_num = 0;
uint8_t txBuffidx = 0;
adi_eth_BufDesc_t       txBufDesc[BUFF_DESC_COUNT];
adi_eth_BufDesc_t       rxBufDesc[BUFF_DESC_COUNT];

static void txCallback(void *pCBParam, uint32_t Event, void *pArg)
{
    adi_eth_BufDesc_t       *pTxBufDesc;

    pTxBufDesc = (adi_eth_BufDesc_t *)pArg;
    /* Buffer has been written to the ADIN1110 Tx FIFO, we mark it available */
    /* to re-submit to the Tx queue with updated contents. */
    for (uint32_t i = 0; i < BUFF_DESC_COUNT; i++) {
        if (&txBuf[i][0] == pTxBufDesc->pBuf) {
            txBufAvailable[i] = true;
            break;
        }
    }
}

static void rxCallback(void *pCBParam, uint32_t Event, void *pArg)
{
    adin1110_DeviceHandle_t hDevice = (adin1110_DeviceHandle_t)pCBParam;
    adi_eth_BufDesc_t       *pRxBufDesc;
    uint32_t                idx;

    pRxBufDesc = (adi_eth_BufDesc_t *)pArg;
 
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
        txBufAvailable[i] = true;

        //Submit All rx buffersryzen
        rxBufDesc[i].pBuf = &rxBuf[i][0];
        rxBufDesc[i].bufSize = MAX_FRAME_BUF_SIZE;
        rxBufDesc[i].cbFunc = rxCallback;
        
        result = adin1110.submitRxBuffer(&rxBufDesc[i]);
    }

    result = adin1110.enable();
    if(result != ADI_ETH_SUCCESS) Serial.println("Device enable error");

    /* Wait for link to be established */
    Serial.print("Device Configured, waiting for connection...");
    unsigned long prev = millis();
    unsigned long now;
    do
    {
        now = millis();
        if( (now - prev) >= 1000)
        {
          prev = now;
          Serial.print(".");
        }
    } while (linkStatus != ADI_ETH_LINK_STATUS_UP);
}

void loop() {

    adi_eth_Result_e        result;
    unsigned long now;

    now = millis();
    if(now-last_report >= 1000)
    {
      if (txBufAvailable[txBuffidx] && linkStatus == ADI_ETH_LINK_STATUS_UP)
      {
        uint8_t output_data[MAX_FRAME_BUF_SIZE]; //temporary buffer for building data that will be copied to global tx buffer
                 
        for(int i = 0; i < MIN_PAYLOAD_SIZE; i++)
          output_data[i] = outputValue++;

        last_report = now;
        memcpy(&txBuf[txBuffidx][0], frame_header, FRAME_HEADER_SIZE);
        memcpy(&txBuf[txBuffidx][FRAME_HEADER_SIZE], output_data, MIN_PAYLOAD_SIZE);

        txBufDesc[txBuffidx].pBuf = &txBuf[txBuffidx][0];
        txBufDesc[txBuffidx].trxSize = FRAME_HEADER_SIZE+MIN_PAYLOAD_SIZE;
        txBufDesc[txBuffidx].bufSize = MAX_FRAME_BUF_SIZE;
        txBufDesc[txBuffidx].egressCapt = ADI_MAC_EGRESS_CAPTURE_NONE;
        txBufDesc[txBuffidx].cbFunc = txCallback;

        txBufAvailable[txBuffidx] = false;
        
        result = adin1110.submitTxBuffer(&txBufDesc[txBuffidx]);
        if (result == ADI_ETH_SUCCESS)
        {
            txBuffidx = (txBuffidx + 1) % BUFF_DESC_COUNT;
        }
        else
        {
            /* If Tx buffer submission fails (for example the Tx queue */
            /* may be full), then mark the buffer unavailable.  */
            txBufAvailable[txBuffidx] = true;
        }
      }
      else if(linkStatus != ADI_ETH_LINK_STATUS_UP)
      {
        Serial.println("Waiting for link to resume sending");
      }
      else
      {
        Serial.println("All transmit buffers full");
      }
    }

    now = millis();
    if(now-last_toggle >= 1000)
    {
      digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
      last_toggle = now;
    }
    
    delay(100);
}
