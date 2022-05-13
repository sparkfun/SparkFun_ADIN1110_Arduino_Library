
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
#include "SparkFunBME280.h"
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
BME280 mySensor;



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

    Serial.println("Recieved: ");
    for(int i = 0; i < pRxBufDesc->trxSize; i++)
    {
      Serial.print(pRxBufDesc->pBuf[i]);
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
    Wire.begin();
  
    if (mySensor.beginI2C() == false) //Begin communication over I2C
    {
      Serial.println("The BME280 sensor did not respond. Please check wiring.");
      while(1); //Freeze
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
    DEBUG_RESULT("Device enable error", result, ADI_ETH_SUCCESS);

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

#define diff(a,b,max_diff) ((a>=b+max_diff) || (a<=b-max_diff))

void loop() {

    adi_eth_Result_e        result;
    unsigned long now;
    bool force_report = false;

    float humidity = mySensor.readFloatHumidity();
    float pressure = mySensor.readFloatPressure();
    float alt = mySensor.readFloatAltitudeFeet();
    float temp = mySensor.readTempF();
    int humidity_dec = ( (int)(humidity*100) ) % 100;
    int pressure_dec = ( (int)(pressure*100) ) % 100;
    int alt_dec = ( (int)(alt*100) ) % 100;
    int temp_dec = ( (int)(temp*100) ) % 100;

    if(diff(humidity, reported_humidity, 2)) force_report = true;
    if(diff(pressure, reported_pressure, 300)) force_report = true;
    if(diff(alt, reported_alt, 30)) force_report = true;
    if(diff(temp, reported_temp, 1)) force_report = true;

    now = millis();
    if(now-last_report >= 5000 || force_report)
    {
      if (txBufAvailable[txBuffidx] && linkStatus == ADI_ETH_LINK_STATUS_UP)
      {
        char output_string[MAX_FRAME_BUF_SIZE];
        uint16_t dataLength;
                 
        sprintf(output_string, "Humidity: %d.%02d\r\nPressure: %d\r\nAlt: %d.%02d\r\nTemp: %d.%02d", (int)humidity, humidity_dec, (int)pressure, (int)alt, alt_dec, (int)temp, temp_dec);
        reported_humidity = humidity;
        reported_pressure = pressure;
        reported_alt = alt;
        reported_temp = temp;
        last_report = now;
        dataLength = strnlen(output_string, MAX_FRAME_BUF_SIZE);
        output_string[dataLength++] = '\0';
        while(dataLength < MIN_PAYLOAD_SIZE) output_string[dataLength++] = '\0';
        memcpy(&txBuf[txBuffidx][0], frame_header, FRAME_HEADER_SIZE);
        memcpy(&txBuf[txBuffidx][FRAME_HEADER_SIZE], output_string, dataLength);

        txBufDesc[txBuffidx].pBuf = &txBuf[txBuffidx][0];
        txBufDesc[txBuffidx].trxSize = FRAME_HEADER_SIZE+dataLength;
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
