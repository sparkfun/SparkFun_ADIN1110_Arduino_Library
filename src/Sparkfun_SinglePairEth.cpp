#include "Sparkfun_SinglePairEth.h"


//Map between device handles and class instances to allow the static (c function pointer compatible)
//functions to there respective non-static member functions
std::map<adin1110_DeviceHandle_t, SinglePairEth *> SinglePairEth::devices;

//The next three function are static member functions. Static member functions are needed to get a function
//pointer that we can call in the C function that attaches the interrupt in the driver.
//The devices map is used to locate the appropriate object to call the desired non-static callback member
//This is a bit of a hack, but it is the best I could come up with
void SinglePairEth::linkCallback_C_Compatible(void *pCBParam, uint32_t Event, void *pArg)
{
    adin1110_DeviceHandle_t device = reinterpret_cast<adin1110_DeviceHandle_t>(pCBParam);
    SinglePairEth * self = devices[device];
    if(self)
    {
        self->linkCallback(pCBParam, Event, pArg);
    }
    
}
void SinglePairEth::txCallback_C_Compatible(void *pCBParam, uint32_t Event, void *pArg)
{
    adin1110_DeviceHandle_t device = reinterpret_cast<adin1110_DeviceHandle_t>(pCBParam);
    SinglePairEth * self = devices[device];
    if(self)
    {
        self->txCallback(pCBParam, Event, pArg);
    }
}
void SinglePairEth::rxCallback_C_Compatible(void *pCBParam, uint32_t Event, void *pArg)
{
    adin1110_DeviceHandle_t device = reinterpret_cast<adin1110_DeviceHandle_t>(pCBParam);
    SinglePairEth * self = devices[device];
    if(self)
    {
        self->rxCallback(pCBParam, Event, pArg);
    }
}


void SinglePairEth::txCallback(void *pCBParam, uint32_t Event, void *pArg)
{
    adi_eth_BufDesc_t       *pTxBufDesc = (adi_eth_BufDesc_t *)pArg;

    /* Buffer has been written to the ADIN1110 Tx FIFO, we mark it available */
    /* to re-submit to the Tx queue with updated contents. */
    for (uint32_t i = 0; i < kNumBufs; i++)
    {
        if (&txBuf[i][0] == pTxBufDesc->pBuf)
        {
            txBufAvailable[i] = true;
            break;
        }
    }
    //Call user Callback
    if (userTxCallback)
    {
        userTxCallback();
    }

}

void SinglePairEth::rxCallback(void *pCBParam, uint32_t Event, void *pArg)
{
    adi_eth_BufDesc_t       *pRxBufDesc = (adi_eth_BufDesc_t *)pArg;
    rxSinceLastCheck = true;

    uint32_t i;
    for (i = 0; i < kNumBufs; i++)
    {
        if (&rxBuf[i][0] == pRxBufDesc->pBuf)
        {
            rxBufAvailable[i] = true;
            break;
        }
    }
    //Call user Callback
    if (userRxCallback && (pRxBufDesc->trxSize > kFrameHeaderSize) )
    {
        userRxCallback(&pRxBufDesc->pBuf[kFrameHeaderSize], (pRxBufDesc->trxSize - kFrameHeaderSize));
        submitRxBuffer(pRxBufDesc);
        rxBufAvailable[i] = false;
    }
}

void SinglePairEth::linkCallback(void *pCBParam, uint32_t Event, void *pArg)
{
    linkStatus = *(adi_eth_LinkStatus_e *)pArg;
    //call user callback
    if (userLinkCallback)
    {
        userLinkCallback(linkStatus);
    }
}

adi_eth_Result_e SinglePairEth::begin(uint8_t cs_pin)
{
    adi_eth_Result_e result;

    result = SinglePairEth_Raw::begin(cs_pin);
    devices[hDevice] = this;
    if(result == ADI_ETH_SUCCESS)
    {
        result =  enableDefaultBehavior();
    }

    return result;

}

adi_eth_Result_e SinglePairEth::begin(uint8_t status, uint8_t interrupt, uint8_t reset, uint8_t chip_select)
{
    adi_eth_Result_e result;

    result = SinglePairEth_Raw::begin(status, interrupt, reset, chip_select);
    devices[hDevice] = this;
    if(result == ADI_ETH_SUCCESS)
    {
        result =  enableDefaultBehavior();
    }

    return result;
}

adi_eth_Result_e SinglePairEth::enableDefaultBehavior()
{
    adi_eth_Result_e result = ADI_ETH_SUCCESS;
    int i = 0;

    if(result == ADI_ETH_SUCCESS)
    {
        result = addAddressFilter(&macAddr[0][0], NULL, 0);
    }

    if(result == ADI_ETH_SUCCESS)
    {
        result = addAddressFilter(&macAddr[1][0], NULL, 0);
    }

    if(result == ADI_ETH_SUCCESS)
    {
        result = syncConfig();
    }

    if(result == ADI_ETH_SUCCESS)
    {
        result = registerCallback(linkCallback_C_Compatible, ADI_MAC_EVT_LINK_CHANGE);
        //Other stuff I was trying, delete before release
        // adi_eth_Callback_t cb = [obj=this](void *pCBParam, uint32_t Event, void *pArg) {
        //     obj->linkCallback(pCBParam, Event, pArg);
        // };
        // result = registerCallback(cb, ADI_MAC_EVT_LINK_CHANGE);
        // auto cpp_fun = std::bind(&SinglePairEth::linkCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);
        // auto callback = static_cast<adi_eth_Callback_t>(cpp_fun);
        // registerCallback(callback, ADI_MAC_EVT_LINK_CHANGE);
        //result = registerCallback((adi_eth_Callback_t)(std::bind(&SinglePairEth::linkCallback, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)), ADI_MAC_EVT_LINK_CHANGE);
    }

    for (uint32_t i = 0; i < kNumBufs; i++)
    {
        if(result != ADI_ETH_SUCCESS)
        {
            break;
        }

        //All tx buffers start available to write, no rx buffers start available to read
        txBufAvailable[i] = true;
        rxBufAvailable[i] = false;
        rxBufDesc[i].pBuf = &rxBuf[i][0];
        rxBufDesc[i].bufSize = kMaxBufFrameSize;
        rxBufDesc[i].cbFunc = rxCallback_C_Compatible;

        result = submitRxBuffer(&rxBufDesc[i]);
    }

    if(result == ADI_ETH_SUCCESS)
    {
        result = enable();
    }

    return result;
}

adi_eth_Result_e SinglePairEth::sendData(uint8_t *data, uint16_t dataLen)
{
    adi_eth_Result_e result;

    if(dataLen + kFrameHeaderSize > kFrameSize)
    {
        return ADI_ETH_INVALID_PARAM;
    }

    memcpy(&txBuf[txBufIdx][0], ethFrameHeader, kFrameHeaderSize);
    memcpy(&txBuf[txBufIdx][kFrameHeaderSize], data, dataLen);
    uint16_t trasmitLength = kFrameHeaderSize + dataLen;
    //pad with 0's to mininmum transmit length
    while(trasmitLength < kMinPayloadSize) txBuf[txBufIdx][trasmitLength++] = 0;

    txBufDesc[txBufIdx].pBuf = &txBuf[txBufIdx][0];
    txBufDesc[txBufIdx].trxSize = trasmitLength;
    txBufDesc[txBufIdx].bufSize = kMaxBufFrameSize;
    txBufDesc[txBufIdx].egressCapt = ADI_MAC_EGRESS_CAPTURE_NONE;
    txBufDesc[txBufIdx].cbFunc = txCallback_C_Compatible;

    txBufAvailable[txBufIdx] = false;
    
    result = submitTxBuffer(&txBufDesc[txBufIdx]);
    if (result == ADI_ETH_SUCCESS)
    {
        txBufIdx = (txBufIdx + 1) % kNumBufs;
    }
    else
    {
        /* If Tx buffer submission fails (for example the Tx queue */
        /* may be full), then mark the buffer unavailable.  */
        txBufAvailable[txBufIdx] = true;
    }
    return result;
}
uint16_t SinglePairEth::getRxData(uint8_t *data, uint16_t dataLen)
{
    bool rxDataAvailable = false;
    for(int i = 0; i < kNumBufs; i++)
    {
        if(rxBufAvailable[rxBufIdx])
        {
            rxDataAvailable = true;
            break;
        }
        rxBufIdx = (rxBufIdx + 1) % kNumBufs;
    }
    if(rxDataAvailable)
    {
        uint16_t cpyLen = rxBufDesc[rxBufIdx].trxSize - kFrameHeaderSize;
        cpyLen = (cpyLen < dataLen) ? cpyLen : dataLen;
        memcpy(data, (char *)&(rxBufDesc[rxBufIdx].pBuf[kFrameHeaderSize]), cpyLen);
        submitRxBuffer(&rxBufDesc[rxBufIdx]);
        rxBufAvailable[rxBufIdx] = false;
        rxSinceLastCheck = false;
        return cpyLen;
    }
    return 0;
}

bool SinglePairEth::getRxAvailable()
{
    return rxSinceLastCheck;
}

adi_eth_Result_e SinglePairEth::setRxCallback( void (*cbFunc)(uint8_t *, uint16_t) )
{
    userRxCallback = cbFunc;
    return ADI_ETH_SUCCESS;
}
adi_eth_Result_e SinglePairEth::setTxCallback( void (*cbFunc)() )
{
    userTxCallback = cbFunc;
    return ADI_ETH_SUCCESS;
}
adi_eth_Result_e SinglePairEth::setLinkCallback( void (*cbFunc)(adi_eth_LinkStatus_e) )
{
    userLinkCallback = cbFunc;
    return ADI_ETH_SUCCESS;
}
