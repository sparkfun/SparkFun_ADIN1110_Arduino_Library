/*
* This code is released under the [MIT License](http://opensource.org/licenses/MIT).
* Please review the LICENSE.md file included with this example. If you have any questions 
* or concerns with licensing, please contact techsupport@sparkfun.com.
* Distributed as-is; no warranty is given.
*/

#include "sfe_spe_advanced.h"

const int ADIN1110_INIT_ITER = 5;

adi_eth_Result_e sfe_spe_advanced::begin(uint8_t cs_pin)
{
    adi_eth_Result_e result;
    BSP_ConfigSystemCS(cs_pin);
    if(BSP_InitSystem())
    {
        return ADI_ETH_DEVICE_UNINITIALIZED;
    }

    BSP_HWReset(true);

    for (uint32_t i = 0; i < ADIN1110_INIT_ITER; i++)
    {
        result = init(&drvConfig);
        if (result == ADI_ETH_SUCCESS)
        {
            break;
        }
    }

    return result;
}

adi_eth_Result_e sfe_spe_advanced::begin(uint8_t status, uint8_t interrupt, uint8_t reset, uint8_t chip_select)
{
    adi_eth_Result_e result;
    BSP_ConfigSystem(status, interrupt, reset, chip_select);
    if(BSP_InitSystem())
    {
        return ADI_ETH_DEVICE_UNINITIALIZED;
    }

    BSP_HWReset(true);

    for (uint32_t i = 0; i < ADIN1110_INIT_ITER; i++)
    {
        result = init(&drvConfig);
        if (result == ADI_ETH_SUCCESS)
        {
            break;
        }
    }

    return result;
}

adi_eth_Result_e    sfe_spe_advanced::init                    (adin1110_DriverConfig_t *pCfg) 
{
    return adin1110_Init(hDevice, pCfg);
}

adi_eth_Result_e    sfe_spe_advanced::unInit                  ()
{
    return adin1110_UnInit(hDevice);
}

adi_eth_Result_e    sfe_spe_advanced::getDeviceId             (adin1110_DeviceId_t *pDevId)
{
    return adin1110_GetDeviceId(hDevice, pDevId);
}

adi_eth_Result_e    sfe_spe_advanced::enable                  ()
{
    return adin1110_Enable(hDevice);
}

adi_eth_Result_e    sfe_spe_advanced::disable                 ()
{
    return adin1110_Disable(hDevice);
}

adi_eth_Result_e    sfe_spe_advanced::reset                   (adi_eth_ResetType_e resetType)
{
    return adin1110_Reset(hDevice, resetType);
}

adi_eth_Result_e    sfe_spe_advanced::syncConfig              ()
{
    return adin1110_SyncConfig(hDevice);
}

adi_eth_Result_e    sfe_spe_advanced::getLinkStatus           (adi_eth_LinkStatus_e *linkStatus)
{
    return adin1110_GetLinkStatus(hDevice, linkStatus);
}

adi_eth_Result_e    sfe_spe_advanced::getStatCounters         (adi_eth_MacStatCounters_t *stat)
{
    return adin1110_GetStatCounters(hDevice, stat);
}

adi_eth_Result_e    sfe_spe_advanced::ledEn                   (bool enable)
{
    return adin1110_LedEn(hDevice, enable);
}

adi_eth_Result_e    sfe_spe_advanced::setLoopbackMode         (adi_phy_LoopbackMode_e loopbackMode)
{
    return adin1110_SetLoopbackMode(hDevice, loopbackMode);
}

adi_eth_Result_e    sfe_spe_advanced::setTestMode             (adi_phy_TestMode_e testMode)
{
    return adin1110_SetTestMode(hDevice, testMode);
}

adi_eth_Result_e    sfe_spe_advanced::addAddressFilter        (uint8_t *macAddr, uint8_t *macAddrMask, uint32_t priority)
{
    return adin1110_AddAddressFilter(hDevice, macAddr, macAddrMask, priority);
}

adi_eth_Result_e    sfe_spe_advanced::clearAddressFilter      (uint32_t addrIndex)
{
    return adin1110_ClearAddressFilter(hDevice, addrIndex);
}

adi_eth_Result_e    sfe_spe_advanced::submitTxBuffer          (adi_eth_BufDesc_t *pBufDesc)
{
    return adin1110_SubmitTxBuffer(hDevice, pBufDesc);
}

adi_eth_Result_e    sfe_spe_advanced::submitRxBuffer          (adi_eth_BufDesc_t *pBufDesc)
{
    return adin1110_SubmitRxBuffer(hDevice, pBufDesc);
}

#if defined(ADI_MAC_ENABLE_RX_QUEUE_HI_PRIO)
adi_eth_Result_e    TwoWire_Eth::submitRxBufferHp        (adi_eth_BufDesc_t *pBufDesc)
{
    return adin1110_SubmitRxBufferHp(hDevice, pBufDesc);
}
#endif

adi_eth_Result_e    sfe_spe_advanced::setPromiscuousMode      (bool bFlag)
{
    return adin1110_SetPromiscuousMode(hDevice, bFlag);
}

adi_eth_Result_e    sfe_spe_advanced::getPromiscuousMode      (bool *pFlag)
{
    return adin1110_GetPromiscuousMode(hDevice, pFlag);
}

#if defined(SPI_OA_EN)
adi_eth_Result_e    sfe_spe_advanced::setChunkSize            (adi_mac_OaCps_e cps)
{
    return adin1110_SetChunkSize(hDevice, cps);
}

adi_eth_Result_e    sfe_spe_advanced::getChunkSize            (adi_mac_OaCps_e *pCps)
{
    return adin1110_GetChunkSize(hDevice, pCps);
}
#endif

adi_eth_Result_e    sfe_spe_advanced::setCutThroughMode       (bool txcte, bool rxcte)
{
    return adin1110_SetCutThroughMode(hDevice, txcte, rxcte);
}

adi_eth_Result_e    sfe_spe_advanced::getCutThroughMode       (bool *pTxcte, bool *pRxcte)
{
    return adin1110_GetCutThroughMode(hDevice, pTxcte, pRxcte);
}

adi_eth_Result_e    sfe_spe_advanced::setFifoSizes            (adi_mac_FifoSizes_t fifoSizes)
{
    return adin1110_SetFifoSizes(hDevice, fifoSizes);
}

adi_eth_Result_e    sfe_spe_advanced::getFifoSizes            (adi_mac_FifoSizes_t *pFifoSizes)
{
    return adin1110_GetFifoSizes(hDevice, pFifoSizes);
}

adi_eth_Result_e    sfe_spe_advanced::clearFifos              (adi_mac_FifoClrMode_e clearMode)
{
    return adin1110_ClearFifos(hDevice, clearMode);
}

adi_eth_Result_e    sfe_spe_advanced::tsEnable                (adi_mac_TsFormat_e format)
{
    return adin1110_TsEnable(hDevice, format);
}

adi_eth_Result_e    sfe_spe_advanced::tsClear                 ()
{
    return adin1110_TsClear(hDevice);
}

adi_eth_Result_e    sfe_spe_advanced::tsTimerStart            (adi_mac_TsTimerConfig_t *pTimerConfig)
{
    return adin1110_TsTimerStart(hDevice, pTimerConfig);
}

adi_eth_Result_e    sfe_spe_advanced::tsTimerStop             ()
{
    return adin1110_TsTimerStop(hDevice);
}

adi_eth_Result_e    sfe_spe_advanced::tsSetTimerAbsolute      (uint32_t seconds, uint32_t nanoseconds)
{
    return adin1110_TsSetTimerAbsolute(hDevice, seconds, nanoseconds);
}

adi_eth_Result_e    sfe_spe_advanced::tsSyncClock             (int64_t tError, uint64_t referenceTimeNsDiff, uint64_t localTimeNsDiff)
{
    return adin1110_TsSyncClock(hDevice, tError, referenceTimeNsDiff, localTimeNsDiff);
}

adi_eth_Result_e    sfe_spe_advanced::tsGetExtCaptTimestamp   (adi_mac_TsTimespec_t *pCapturedTimespec)
{
    return adin1110_TsGetExtCaptTimestamp(hDevice, pCapturedTimespec);
}

adi_eth_Result_e    sfe_spe_advanced::tsGetEgressTimestamp    (adi_mac_EgressCapture_e egressReg, adi_mac_TsTimespec_t *pCapturedTimespec)
{
    return adin1110_TsGetEgressTimestamp(hDevice, egressReg, pCapturedTimespec);
}

adi_eth_Result_e    sfe_spe_advanced::tsConvert               (uint32_t timestampLowWord, uint32_t timestampHighWord, adi_mac_TsFormat_e format, adi_mac_TsTimespec_t *pTimespec)
{
    return adin1110_TsConvert(timestampLowWord, timestampHighWord, format, pTimespec);
}

int64_t             sfe_spe_advanced::tsSubtract              (adi_mac_TsTimespec_t *pTsA, adi_mac_TsTimespec_t *pTsB)
{
    return adin1110_TsSubtract(pTsA, pTsB);
}

adi_eth_Result_e    sfe_spe_advanced::registerCallback        (adi_eth_Callback_t cbFunc, adi_mac_InterruptEvt_e cbEvent)
{
    // return adin1110_RegisterCallback(hDevice, cbFunc, cbEvent);
    return adin1110_RegisterCallback(hDevice, cbFunc, cbEvent);
}

adi_eth_Result_e    sfe_spe_advanced::setUserContext          (void *pContext)
{
    return adin1110_SetUserContext(hDevice, pContext);
}

void *              sfe_spe_advanced::getUserContext          ()
{
    return adin1110_GetUserContext(hDevice);
}

adi_eth_Result_e    sfe_spe_advanced::writeRegister           (uint16_t regAddr, uint32_t regData)
{
    return adin1110_WriteRegister(hDevice, regAddr, regData);
}

adi_eth_Result_e    sfe_spe_advanced::readRegister            (uint16_t regAddr, uint32_t *regData)
{
    return adin1110_ReadRegister(hDevice, regAddr, regData);
}

adi_eth_Result_e    sfe_spe_advanced::phyWrite                (uint32_t regAddr, uint16_t regData)
{
    return adin1110_PhyWrite(hDevice, regAddr, regData);
}

adi_eth_Result_e    sfe_spe_advanced::phyRead                 (uint32_t regAddr, uint16_t *regData)
{
    return adin1110_PhyRead(hDevice, regAddr, regData);
}

adi_eth_Result_e    sfe_spe_advanced::getMseLinkQuality      (adi_phy_MseLinkQuality_t *mseLinkQuality)
{
    return adin1110_GetMseLinkQuality(hDevice, mseLinkQuality);
}

adi_eth_Result_e    sfe_spe_advanced::frameGenEn             (bool enable)
{
    return adin1110_FrameGenEn(hDevice, enable);
}

adi_eth_Result_e    sfe_spe_advanced::frameGenSetMode        (adi_phy_FrameGenMode_e mode)
{
    return adin1110_FrameGenSetMode(hDevice, mode);
}

adi_eth_Result_e    sfe_spe_advanced::frameGenSetFrameCnt    (uint32_t frameCnt)
{
    return adin1110_FrameGenSetFrameCnt(hDevice, frameCnt);
}

adi_eth_Result_e    sfe_spe_advanced::frameGenSetFramePayload(adi_phy_FrameGenPayload_e payload)
{
    return adin1110_FrameGenSetFramePayload(hDevice, payload);
}

adi_eth_Result_e    sfe_spe_advanced::frameGenSetFrameLen    (uint16_t frameLen)
{
    return adin1110_FrameGenSetFrameLen(hDevice, frameLen);
}

adi_eth_Result_e    sfe_spe_advanced::frameGenSetIfgLen      (uint16_t ifgLen)
{
    return adin1110_FrameGenSetIfgLen(hDevice, ifgLen);
}

adi_eth_Result_e    sfe_spe_advanced::frameGenRestart        ()
{
    return adin1110_FrameGenRestart(hDevice);
}

adi_eth_Result_e    sfe_spe_advanced::frameGenDone           (bool *fgDone)
{
    return adin1110_FrameGenDone(hDevice, fgDone);
}

adi_eth_Result_e    sfe_spe_advanced::frameChkEn             (bool enable)
{
    return adin1110_FrameChkEn(hDevice, enable);
}

adi_eth_Result_e    sfe_spe_advanced::frameChkSourceSelect   (adi_phy_FrameChkSource_e source)
{
    return adin1110_FrameChkSourceSelect(hDevice, source);
}

adi_eth_Result_e    sfe_spe_advanced::frameChkReadFrameCnt   (uint32_t *cnt)
{
    return adin1110_FrameChkReadFrameCnt(hDevice, cnt);
}

adi_eth_Result_e    sfe_spe_advanced::frameChkReadRxErrCnt   (uint16_t *cnt)
{
    return adin1110_FrameChkReadRxErrCnt(hDevice, cnt);
}

adi_eth_Result_e    sfe_spe_advanced::frameChkReadErrorCnt   (adi_phy_FrameChkErrorCounters_t *cnt)
{
    return adin1110_FrameChkReadErrorCnt(hDevice, cnt);
}
