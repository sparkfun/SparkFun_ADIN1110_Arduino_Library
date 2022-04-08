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
#include <stdbool.h>
#include "adin1110.h"

/* Extra 4 bytes for FCS and 2 bytes for the frame header */
#define MAX_FRAME_BUF_SIZE  (MAX_FRAME_SIZE + 4 + 2)

#define TEST_FRAMES_COUNT   (2)

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


#define FRAME_COUNT         (20000)

#define ADIN1110_INIT_ITER  (5)

/* Example configuration */
uint8_t devMem[ADIN1110_DEVICE_SIZE];

adin1110_DriverConfig_t drvConfig = {
    .pDevMem    = (void *)devMem,
    .devMemSize = sizeof(devMem),
    .fcsCheckEn = false,
};

void setup() 
{
    adi_eth_Result_e                    result;
    adin1110_DeviceStruct_t             dev;
    adin1110_DeviceHandle_t             hDevice = &dev;
    uint16_t                            fcRxErrCnt;
    uint32_t                            fcFrameCnt;
    adi_phy_FrameChkErrorCounters_t     fcErrCnt;
    bool                                fgDone;
    Serial.begin(115200);
    if (BSP_InitSystem())
    {
        /* Hardware initialization error, blink all LEDs on board */
        while (1)
        {
            BSP_LedToggleAll();
            for (uint32_t i = 0; i < 10000000; i++)
              ;
        }
    }

    result = adin1110_Init(hDevice, &drvConfig);
    DEBUG_RESULT("adin1110_Init", result, ADI_ETH_SUCCESS);

    /* Confirm device configuration                          */
    result = adin1110_SyncConfig(hDevice);
    DEBUG_RESULT("adin1110_SyncConfig", result, ADI_ETH_SUCCESS);

    /* Enable the switch */
    result = adin1110_Enable(hDevice);
    DEBUG_RESULT("adin1110_Enable", result, ADI_ETH_SUCCESS);

    fcRxErrCnt = 0;
    fcFrameCnt = 0;
    fgDone = false;

    /* Enable frame checker and select the correct source */
    result = adin1110_FrameChkEn(hDevice, true);
    DEBUG_RESULT("adin1110_FrameChkEn", result, ADI_ETH_SUCCESS);
    result = adin1110_FrameChkSourceSelect(hDevice, ADI_PHY_FRAME_CHK_SOURCE_PHY);
    DEBUG_RESULT("adin1110_FrameChkSourceSelect", result, ADI_ETH_SUCCESS);

    /* Reset counters by reading RX_ERR_CNT registers */
    result = adin1110_FrameChkReadRxErrCnt(hDevice, &fcRxErrCnt);
    DEBUG_RESULT("adin1110_FrameChkReadRxErrCnt", result, ADI_ETH_SUCCESS);

    /* Set PCS loopback mode, causing all generated frames */
    /* to be looped backfor checking by the frame checker. */
    result = adin1110_SetLoopbackMode(hDevice, ADI_PHY_LOOPBACK_PCS);
    DEBUG_RESULT("adin1110_SetLoopbackMode", result, ADI_ETH_SUCCESS);

    /* Enable frame generator and configure burst mode, then start generating frames */
    result = adin1110_FrameGenEn(hDevice, true);
    DEBUG_RESULT("adin1110_FrameGenEn", result, ADI_ETH_SUCCESS);
    result = adin1110_FrameGenSetMode(hDevice, ADI_PHY_FRAME_GEN_MODE_BURST);
    DEBUG_RESULT("adin1110_FrameGenSetMode", result, ADI_ETH_SUCCESS);
    result = adin1110_FrameGenSetFrameCnt(hDevice, FRAME_COUNT);
    DEBUG_RESULT("adin1110_FrameGenSetFrameCnt", result, ADI_ETH_SUCCESS);
    result = adin1110_FrameGenRestart(hDevice);
    DEBUG_RESULT("adin1110_FrameGenRestart", result, ADI_ETH_SUCCESS);

    /* Wait for indication that all frames were generated */
    do
    {
        result = adin1110_FrameGenDone(hDevice, &fgDone);
        DEBUG_RESULT("adin1110_FrameGenDone", result, ADI_ETH_SUCCESS);
    } while ((result != ADI_ETH_SUCCESS) || !fgDone);

    /* Read results, note RX_ERR_CNT is read first to latch all counter values. */
    /* This is required before reading any of the other counter registers.      */
    result = adin1110_FrameChkReadRxErrCnt(hDevice, &fcRxErrCnt);
    DEBUG_RESULT("adin1110_FrameChkReadRxErrCnt", result, ADI_ETH_SUCCESS);
    result = adin1110_FrameChkReadFrameCnt(hDevice, &fcFrameCnt);
    DEBUG_RESULT("adin1110_FrameChkReadFrameCnt", result, ADI_ETH_SUCCESS);
    result = adin1110_FrameChkReadErrorCnt(hDevice, &fcErrCnt);
    DEBUG_RESULT("adin1110_FrameChkReadErrorCnt", result, ADI_ETH_SUCCESS);

    DEBUG_MESSAGE("Frame Generator/Checker Results:");
    DEBUG_MESSAGE("    (FG) Sent:           %d frames", FRAME_COUNT);
    DEBUG_MESSAGE("    (FC) Received:       %d frames", fcFrameCnt);
    DEBUG_MESSAGE("         Rx Errors             = %d", fcRxErrCnt);
    DEBUG_MESSAGE("           LEN_ERR_CNT         = %d", fcErrCnt.LEN_ERR_CNT);
    DEBUG_MESSAGE("           ALGN_ERR_CNT        = %d", fcErrCnt.ALGN_ERR_CNT);
    DEBUG_MESSAGE("           SYMB_ERR_CNT        = %d", fcErrCnt.SYMB_ERR_CNT);
    DEBUG_MESSAGE("           OSZ_CNT             = %d", fcErrCnt.OSZ_CNT);
    DEBUG_MESSAGE("           USZ_CNT             = %d", fcErrCnt.USZ_CNT);
    DEBUG_MESSAGE("           ODD_CNT             = %d", fcErrCnt.ODD_CNT);
    DEBUG_MESSAGE("           ODD_PRE_CNT         = %d", fcErrCnt.ODD_PRE_CNT);
    DEBUG_MESSAGE("           FALSE_CARRIER_CNT   = %d", fcErrCnt.FALSE_CARRIER_CNT);

    result = adin1110_FrameGenEn(hDevice, false);
    DEBUG_RESULT("adin1110_FrameGenEn", result, ADI_ETH_SUCCESS);

    result = adin1110_Disable(hDevice);
    DEBUG_RESULT("adin1110_Disable", result, ADI_ETH_SUCCESS);

    result = adin1110_UnInit(hDevice);
    DEBUG_RESULT("adin1110_UnInit", result, ADI_ETH_SUCCESS);

}

void loop () {}