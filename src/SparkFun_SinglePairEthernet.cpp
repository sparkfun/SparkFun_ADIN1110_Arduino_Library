/*
* This code is released under the [MIT License](http://opensource.org/licenses/MIT).
* Please review the LICENSE.md file included with this example. If you have any questions 
* or concerns with licensing, please contact techsupport@sparkfun.com.
* Distributed as-is; no warranty is given.
*/
#include "SparkFun_SinglePairEthernet.h"

//The next three function are static member functions. Static member functions are needed to get a function
//pointer that we can shove into the C function that attaches the interrupt in the driver.
void SinglePairEthernet::linkCallback_C_Compatible(void *pCBParam, uint32_t Event, void *pArg)
{
    adin1110_DeviceHandle_t device = reinterpret_cast<adin1110_DeviceHandle_t>(pCBParam);
    SinglePairEthernet * self = reinterpret_cast<SinglePairEthernet *>(adin1110_GetUserContext(device));
    if(self)
    {
        self->linkCallback(pCBParam, Event, pArg);
    }
    
}
void SinglePairEthernet::txCallback_C_Compatible(void *pCBParam, uint32_t Event, void *pArg)
{
    adin1110_DeviceHandle_t device = reinterpret_cast<adin1110_DeviceHandle_t>(pCBParam);
    SinglePairEthernet * self = reinterpret_cast<SinglePairEthernet *>(adin1110_GetUserContext(device));
    if(self)
    {
        self->txCallback(pCBParam, Event, pArg);
    }
}
void SinglePairEthernet::rxCallback_C_Compatible(void *pCBParam, uint32_t Event, void *pArg)
{
    adin1110_DeviceHandle_t device = reinterpret_cast<adin1110_DeviceHandle_t>(pCBParam);
    SinglePairEthernet * self = reinterpret_cast<SinglePairEthernet *>(adin1110_GetUserContext(device));
    if(self)
    {
        self->rxCallback(pCBParam, Event, pArg);
    }
}

void SinglePairEthernet::txCallback(void *pCBParam, uint32_t Event, void *pArg)
{
    adi_eth_BufDesc_t       *pTxBufDesc = (adi_eth_BufDesc_t *)pArg;

    /* Buffer has been written to the ADIN1110 Tx FIFO, we mark it available */
    /* to re-submit to the Tx queue with updated contents. */
    for (uint32_t i = 0; i < SPE_NUM_BUFS; i++)
    {
        if (&txBuf[i][0] == pTxBufDesc->pBuf)
        {
            txBufAvailable[i] = true;
            break;
        }
    }
}

void SinglePairEthernet::rxCallback(void *pCBParam, uint32_t Event, void *pArg)
{
    adi_eth_BufDesc_t       *pRxBufDesc = (adi_eth_BufDesc_t *)pArg;
    rxSinceLastCheck = true;

    uint32_t i;
    for (i = 0; i < SPE_NUM_BUFS; i++)
    {
        if (&rxBuf[i][0] == pRxBufDesc->pBuf)
        {
            rxBufAvailable[i] = true;
            break;
        }
    }

    //Call user Callback
    if (userRxCallback && (pRxBufDesc->trxSize > SPE_FRAME_HEADER_SIZE) )
    {
        userRxCallback(&pRxBufDesc->pBuf[SPE_FRAME_HEADER_SIZE], (pRxBufDesc->trxSize - SPE_FRAME_HEADER_SIZE), &pRxBufDesc->pBuf[SPE_MAC_SIZE] );
        submitRxBuffer(pRxBufDesc);
        rxBufAvailable[i] = false;
    }
}

void SinglePairEthernet::linkCallback(void *pCBParam, uint32_t Event, void *pArg)
{
    linkStatus = *(adi_eth_LinkStatus_e *)pArg;
    //call user callback
    if (userLinkCallback)
    {
        
        userLinkCallback(linkStatus == ADI_ETH_LINK_STATUS_UP);
    }
}

bool SinglePairEthernet::begin(uint8_t *mac, uint8_t cs_pin)
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

bool SinglePairEthernet::begin(uint8_t *mac, uint8_t status, uint8_t interrupt, uint8_t reset, uint8_t chip_select)
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

adi_eth_Result_e SinglePairEthernet::enableDefaultBehavior()
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

    for (uint32_t i = 0; i < SPE_NUM_BUFS; i++)
    {
        if(result != ADI_ETH_SUCCESS)
        {
            break;
        }

        //All tx buffers start available to write, no rx buffers start available to read
        txBufAvailable[i] = true;
        rxBufAvailable[i] = false;
        rxBufDesc[i].pBuf = &rxBuf[i][0];
        rxBufDesc[i].bufSize = SPE_MAX_BUF_FRAME_SIZE;
        rxBufDesc[i].cbFunc = rxCallback_C_Compatible;

        result = submitRxBuffer(&rxBufDesc[i]);
    }

    if(result == ADI_ETH_SUCCESS)
    {
        result = enable();
    }

    return result;
}

bool SinglePairEthernet::sendData(uint8_t *data, int dataLen)
{
    return sendData(data, dataLen, destMacAddr); //User default destination address
}
   
bool SinglePairEthernet::sendData(uint8_t *data, int dataLen, uint8_t * destMac)
{
    adi_eth_Result_e result;

    if( (dataLen + SPE_FRAME_HEADER_SIZE > SPE_FRAME_SIZE) || (destMac == NULL) )
    {
        return false;
    }
    int transmitLength = 0;

    //Build ethernet frame header
    memcpy(&txBuf[txBufIdx][transmitLength], destMac, SPE_MAC_SIZE);  //Copy dest mac address
    transmitLength += SPE_MAC_SIZE;
    memcpy(&txBuf[txBufIdx][transmitLength], macAddr, SPE_MAC_SIZE);  //Copy own(source) mac address
    transmitLength += SPE_MAC_SIZE;
    txBuf[txBufIdx][transmitLength++] = SPE_ETHERTYPE_IPV4_B0;
    txBuf[txBufIdx][transmitLength++] = SPE_ETHERTYPE_IPV4_B1;
    //Insert provided data
    memcpy(&txBuf[txBufIdx][transmitLength], data, dataLen);
    transmitLength += dataLen;
    //Pad with 0's to mininmum transmit length
    while(transmitLength < SPE_MIN_PAYLOAD_SIZE) txBuf[txBufIdx][transmitLength++] = 0;

    txBufDesc[txBufIdx].pBuf = &txBuf[txBufIdx][0];
    txBufDesc[txBufIdx].trxSize = transmitLength;
    txBufDesc[txBufIdx].bufSize = SPE_MAX_BUF_FRAME_SIZE;
    txBufDesc[txBufIdx].egressCapt = ADI_MAC_EGRESS_CAPTURE_NONE;
    txBufDesc[txBufIdx].cbFunc = txCallback_C_Compatible;

    txBufAvailable[txBufIdx] = false;
    
    result = submitTxBuffer(&txBufDesc[txBufIdx]);
    if (result == ADI_ETH_SUCCESS)
    {
        txBufIdx = (txBufIdx + 1) % SPE_NUM_BUFS;
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
int SinglePairEthernet::getRxData(uint8_t *data, int dataLen, uint8_t *senderMac)
{
    bool rxDataAvailable = false;
    for(int i = 0; i < SPE_NUM_BUFS; i++)
    {
        if(rxBufAvailable[rxBufIdx])
        {
            rxDataAvailable = true;
            break;
        }
        rxBufIdx = (rxBufIdx + 1) % SPE_NUM_BUFS;
    }
    if(rxDataAvailable)
    {
        int cpyLen = rxBufDesc[rxBufIdx].trxSize - SPE_FRAME_HEADER_SIZE;
        cpyLen = (cpyLen < dataLen) ? cpyLen : dataLen;
        memcpy(senderMac, (char *)&(rxBufDesc[rxBufIdx].pBuf[SPE_MAC_SIZE]), SPE_MAC_SIZE); //second set of 6 bytes are senders MAC address
        memcpy(data, (char *)&(rxBufDesc[rxBufIdx].pBuf[SPE_FRAME_HEADER_SIZE]), cpyLen); //data starts 14 bytes in, after the frame header
        submitRxBuffer(&rxBufDesc[rxBufIdx]);
        rxBufAvailable[rxBufIdx] = false;
        rxSinceLastCheck = false;
        return cpyLen;
    }
    return 0;
}

bool SinglePairEthernet::getRxAvailable()
{
    return rxSinceLastCheck;
}


void SinglePairEthernet::setMac(uint8_t * mac)
{
    if(mac)
    {
        memcpy(macAddr, mac, SPE_MAC_SIZE);
    }
}
void SinglePairEthernet::getMac(uint8_t * mac)
{
    if(mac)
    {
        memcpy(mac, macAddr, SPE_MAC_SIZE);
    }
}
void SinglePairEthernet::setDestMac(uint8_t * mac)
{
    if(mac)
    {
        memcpy(mac, destMacAddr, SPE_MAC_SIZE);
    }  
}

bool SinglePairEthernet::indenticalMacs(uint8_t * mac1, uint8_t * mac2)
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

void SinglePairEthernet::setRxCallback( void (*cbFunc)(uint8_t *, int, uint8_t *) )
{
    userRxCallback = cbFunc;
}

void SinglePairEthernet::setLinkCallback( void (*cbFunc)(bool) )
{
    userLinkCallback = cbFunc;
}

bool SinglePairEthernet::getLinkStatus(void)
{
    return (linkStatus == ADI_ETH_LINK_STATUS_UP);
}