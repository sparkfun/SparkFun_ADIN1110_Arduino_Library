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

sfe_spe_advanced adin1110;

void setup() 
{
    adi_eth_Result_e                    result;
    uint16_t                            fcRxErrCnt;
    uint32_t                            fcFrameCnt;
    adi_phy_FrameChkErrorCounters_t     fcErrCnt;
    bool                                fgDone;
    Serial.begin(115200);

    result = adin1110.begin();
    if(result != ADI_ETH_SUCCESS) Serial.println("No MACPHY device found");
    else Serial.println("Adin1110 found!");

    /* Confirm device configuration                          */
    result = adin1110.syncConfig();
    if(result != ADI_ETH_SUCCESS) Serial.println("adin1110_SyncConfig");

    /* Enable the switch */
    result = adin1110.enable();
    if(result != ADI_ETH_SUCCESS) Serial.println("Device enable error");

    fcRxErrCnt = 0;
    fcFrameCnt = 0;
    fgDone = false;

    /* Enable frame checker and select the correct source */
    result = adin1110.frameChkEn(true);
    if(result != ADI_ETH_SUCCESS) Serial.println("adin1110_FrameChkEn");
    result = adin1110.frameChkSourceSelect(ADI_PHY_FRAME_CHK_SOURCE_PHY);
    if(result != ADI_ETH_SUCCESS) Serial.println("adin1110_FrameChkSourceSelect");

    /* Reset counters by reading RX_ERR_CNT registers */
    result = adin1110.frameChkReadRxErrCnt(&fcRxErrCnt);
    if(result != ADI_ETH_SUCCESS) Serial.println("adin1110_FrameChkReadRxErrCnt");

    /* Set PCS loopback mode, causing all generated frames */
    /* to be looped backfor checking by the frame checker. */
    result = adin1110.setLoopbackMode(ADI_PHY_LOOPBACK_PCS);
    if(result != ADI_ETH_SUCCESS) Serial.println("adin1110_SetLoopbackMode");

    /* Enable frame generator and configure burst mode, then start generating frames */
    result = adin1110.frameGenEn(true);
    if(result != ADI_ETH_SUCCESS) Serial.println("adin1110_FrameGenEn");
    result = adin1110.frameGenSetMode(ADI_PHY_FRAME_GEN_MODE_BURST);
    if(result != ADI_ETH_SUCCESS) Serial.println("adin1110_FrameGenSetMode");
    result = adin1110.frameGenSetFrameCnt(FRAME_COUNT);
    if(result != ADI_ETH_SUCCESS) Serial.println("adin1110_FrameGenSetFrameCnt");
    result = adin1110.frameGenRestart();
    if(result != ADI_ETH_SUCCESS) Serial.println("adin1110_FrameGenRestart");

    /* Wait for indication that all frames were generated */
    do
    {
        result = adin1110.frameGenDone(&fgDone);
        if(result != ADI_ETH_SUCCESS) Serial.println("adin1110_FrameGenDone");
    } while ((result != ADI_ETH_SUCCESS) || !fgDone);

    /* Read results, note RX_ERR_CNT is read first to latch all counter values. */
    /* This is required before reading any of the other counter registers.      */
    result = adin1110.frameChkReadRxErrCnt(&fcRxErrCnt);
    if(result != ADI_ETH_SUCCESS) Serial.println("adin1110_FrameChkReadRxErrCnt");
    result = adin1110.frameChkReadFrameCnt(&fcFrameCnt);
    if(result != ADI_ETH_SUCCESS) Serial.println("adin1110_FrameChkReadFrameCnt");
    result = adin1110.frameChkReadErrorCnt(&fcErrCnt);
    if(result != ADI_ETH_SUCCESS) Serial.println("adin1110_FrameChkReadErrorCnt");

    Serial.printf("Frame Generator/Checker Results:\r\n");
    Serial.printf("    (FG) Sent:           %d frames\r\n", FRAME_COUNT);
    Serial.printf("    (FC) Received:       %d frames\r\n", fcFrameCnt);
    Serial.printf("         Rx Errors             = %d\r\n", fcRxErrCnt);
    Serial.printf("           LEN_ERR_CNT         = %d\r\n", fcErrCnt.LEN_ERR_CNT);
    Serial.printf("           ALGN_ERR_CNT        = %d\r\n", fcErrCnt.ALGN_ERR_CNT);
    Serial.printf("           SYMB_ERR_CNT        = %d\r\n", fcErrCnt.SYMB_ERR_CNT);
    Serial.printf("           OSZ_CNT             = %d\r\n", fcErrCnt.OSZ_CNT);
    Serial.printf("           USZ_CNT             = %d\r\n", fcErrCnt.USZ_CNT);
    Serial.printf("           ODD_CNT             = %d\r\n", fcErrCnt.ODD_CNT);
    Serial.printf("           ODD_PRE_CNT         = %d\r\n", fcErrCnt.ODD_PRE_CNT);
    Serial.printf("           FALSE_CARRIER_CNT   = %d\r\n", fcErrCnt.FALSE_CARRIER_CNT);

    result = adin1110.frameGenEn(false);
    if(result != ADI_ETH_SUCCESS) Serial.println("adin1110_FrameGenEn");

    result = adin1110.disable();
    if(result != ADI_ETH_SUCCESS) Serial.println("adin1110_Disable");

    result = adin1110.unInit();
    if(result != ADI_ETH_SUCCESS) Serial.println("adin1110_UnInit");

}

void loop () {}