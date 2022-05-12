#include "SparkFun_SinglePairEthernet.h"

//The next three function are static member functions. Static member functions are needed to get a function
//pointer that we can shove into the C function that attaches the interrupt in the driver.
void SinglePairEth::linkCallback_C_Compatible(void *pCBParam, uint32_t Event, void *pArg)
{
    adin1110_DeviceHandle_t device = reinterpret_cast<adin1110_DeviceHandle_t>(pCBParam);
    SinglePairEth * self = reinterpret_cast<SinglePairEth *>(adin1110_GetUserContext(device));
    if(self)
    {
        self->linkCallback(pCBParam, Event, pArg);
    }
    
}
void SinglePairEth::txCallback_C_Compatible(void *pCBParam, uint32_t Event, void *pArg)
{
    adin1110_DeviceHandle_t device = reinterpret_cast<adin1110_DeviceHandle_t>(pCBParam);
    SinglePairEth * self = reinterpret_cast<SinglePairEth *>(adin1110_GetUserContext(device));
    if(self)
    {
        self->txCallback(pCBParam, Event, pArg);
    }
}
void SinglePairEth::rxCallback_C_Compatible(void *pCBParam, uint32_t Event, void *pArg)
{
    adin1110_DeviceHandle_t device = reinterpret_cast<adin1110_DeviceHandle_t>(pCBParam);
    SinglePairEth * self = reinterpret_cast<SinglePairEth *>(adin1110_GetUserContext(device));
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
        userRxCallback(&pRxBufDesc->pBuf[kFrameHeaderSize], (pRxBufDesc->trxSize - kFrameHeaderSize), &pRxBufDesc->pBuf[kMacSize] );
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
        
        userLinkCallback(linkStatus == ADI_ETH_LINK_STATUS_UP);
    }
}

bool SinglePairEth::begin(uint8_t *mac, uint8_t cs_pin)
{
    adi_eth_Result_e result;

    if(mac)
    {
        setMac(mac);
    }
    result = sfe_spe_advanced::begin(cs_pin);
    setUserContext((void *)this);
    if(result == ADI_ETH_SUCCESS)
    {
        result =  enableDefaultBehavior();
    }

    return (result == ADI_ETH_SUCCESS);

}

bool SinglePairEth::begin(uint8_t *mac, uint8_t status, uint8_t interrupt, uint8_t reset, uint8_t chip_select)
{
    adi_eth_Result_e result;

    if(mac)
    {
        setMac(mac);
    }
    result = sfe_spe_advanced::begin(status, interrupt, reset, chip_select);
    setUserContext((void *)this);
    if(result == ADI_ETH_SUCCESS)
    {
        result =  enableDefaultBehavior();
    }

    return (result == ADI_ETH_SUCCESS);
}

adi_eth_Result_e SinglePairEth::enableDefaultBehavior()
{
    adi_eth_Result_e result = ADI_ETH_SUCCESS;
    int i = 0;

    if(result == ADI_ETH_SUCCESS)
    {
        result = addAddressFilter(macAddr, NULL, 0);
    }

    if(result == ADI_ETH_SUCCESS)
    {
        result = syncConfig();
    }

    if(result == ADI_ETH_SUCCESS)
    {
        result = registerCallback(linkCallback_C_Compatible, ADI_MAC_EVT_LINK_CHANGE);
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

bool SinglePairEth::sendData(uint8_t *data, uint16_t dataLen)
{
    return sendData(data, dataLen, destMacAddr); //User default destination address
}
   
bool SinglePairEth::sendData(uint8_t *data, uint16_t dataLen, uint8_t * destMac)
{
    adi_eth_Result_e result;

    if( (dataLen + kFrameHeaderSize > kFrameSize) || (destMac == NULL) )
    {
        return false;
    }
    uint16_t transmitLength = 0;

    //Build ethernet frame header
    memcpy(&txBuf[txBufIdx][transmitLength], destMac, kMacSize);  //Copy dest mac address
    transmitLength += kMacSize;
    memcpy(&txBuf[txBufIdx][transmitLength], macAddr, kMacSize);  //Copy own(source) mac address
    transmitLength += kMacSize;
    txBuf[txBufIdx][transmitLength++] = kEtherTypeIPv4_b0;
    txBuf[txBufIdx][transmitLength++] = kEtherTypeIPv4_b1;
    //Insert provided data
    memcpy(&txBuf[txBufIdx][transmitLength], data, dataLen);
    transmitLength += dataLen;
    //Pad with 0's to mininmum transmit length
    while(transmitLength < kMinPayloadSize) txBuf[txBufIdx][transmitLength++] = 0;

    txBufDesc[txBufIdx].pBuf = &txBuf[txBufIdx][0];
    txBufDesc[txBufIdx].trxSize = transmitLength;
    txBufDesc[txBufIdx].bufSize = kMaxBufFrameSize;
    txBufDesc[txBufIdx].egressCapt = ADI_MAC_EGRESS_CAPTURE_NONE;
    txBufDesc[txBufIdx].cbFunc = txCallback_C_Compatible;

    txBufAvailable[txBufIdx] = false;
    
    result = submitTxBuffer(&txBufDesc[txBufIdx]);
    if (result == ADI_ETH_SUCCESS)
    {
        txBufIdx = (txBufIdx + 1) % kNumBufs;
        setDestMac(destMac); //save most recently successfully sent mac address as the mac to use if none is provided in future calls
    }
    else
    {
        /* If Tx buffer submission fails (for example the Tx queue */
        /* may be full), then mark the buffer unavailable.  */
        txBufAvailable[txBufIdx] = true;
    }

    return (result == ADI_ETH_SUCCESS);
}
uint16_t SinglePairEth::getRxData(uint8_t *data, uint16_t dataLen, uint8_t *senderMac)
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
        memcpy(senderMac, (char *)&(rxBufDesc[rxBufIdx].pBuf[kMacSize]), kMacSize); //second set of 6 bytes are senders MAC address
        memcpy(data, (char *)&(rxBufDesc[rxBufIdx].pBuf[kFrameHeaderSize]), cpyLen); //data starts 14 bytes in, after the frame header
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


void SinglePairEth::setMac(uint8_t * mac)
{
    if(mac)
    {
        memcpy(macAddr, mac, kMacSize);
    }
}
void SinglePairEth::getMac(uint8_t * mac)
{
    if(mac)
    {
        memcpy(mac, macAddr, kMacSize);
    }
}
void SinglePairEth::setDestMac(uint8_t * mac)
{
    if(mac)
    {
        memcpy(mac, destMacAddr, kMacSize);
    }  
}

bool SinglePairEth::indenticalMacs(uint8_t * mac1, uint8_t * mac2)
{
    if(!mac1 || !mac2)
    {
        return false;
    }
    return( (mac1[0] == mac2[0]) &&
            (mac1[1] == mac2[1]) &&
            (mac1[2] == mac2[2]) &&
            (mac1[3] == mac2[3]) &&
            (mac1[4] == mac2[4]) &&
            (mac1[5] == mac2[5]) );
}

void SinglePairEth::setRxCallback( void (*cbFunc)(uint8_t *, uint16_t, uint8_t *) )
{
    userRxCallback = cbFunc;
}

void SinglePairEth::setLinkCallback( void (*cbFunc)(bool) )
{
    userLinkCallback = cbFunc;
}

bool SinglePairEth::getLinkStatus(void)
{
    return (linkStatus == ADI_ETH_LINK_STATUS_UP);
}