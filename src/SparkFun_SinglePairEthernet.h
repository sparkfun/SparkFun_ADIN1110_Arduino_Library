/*
* This code is released under the [MIT License](http://opensource.org/licenses/MIT).
* Please review the LICENSE.md file included with this example. If you have any questions 
* or concerns with licensing, please contact techsupport@sparkfun.com.
* Distributed as-is; no warranty is given.
*/
#ifndef __Sparkfun_SinglePairEth__
#define __Sparkfun_SinglePairEth__

#include "sfe_spe_advanced.h"
//Default MAC addresses used in Analog Devices examples
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

const int kNumBufs = 4;
const int kFrameSize = 1518;
/* Extra 4 bytes for FCS and 2 bytes for the frame header */
const int kMaxBufFrameSize = (kFrameSize + 4 + 2);
const int kMinPayloadSize = (46);

const int kMacSize = 6;
const int kFrameHeaderSize = (2*kMacSize + 2);
const int kEtherTypeIPv4_b0 = 0x80;
const int kEtherTypeIPv4_b1 = 0x00;

class SinglePairEth : public sfe_spe_advanced
{
private:
    adi_eth_Result_e    enableDefaultBehavior   (); 

    uint8_t rxBuf[kNumBufs][kMaxBufFrameSize] HAL_ALIGNED_ATTRIBUTE(4);
    uint8_t txBuf[kNumBufs][kMaxBufFrameSize] HAL_ALIGNED_ATTRIBUTE(4);
    adi_eth_BufDesc_t rxBufDesc[kNumBufs];
    adi_eth_BufDesc_t txBufDesc[kNumBufs];
    bool txBufAvailable[kNumBufs];
    bool rxBufAvailable[kNumBufs];
    uint32_t txBufIdx;
    uint32_t rxBufIdx;
    bool rxSinceLastCheck;

    uint8_t macAddr[kMacSize] = {MAC_ADDR_0_0, MAC_ADDR_0_1, MAC_ADDR_0_2, MAC_ADDR_0_3, MAC_ADDR_0_4, MAC_ADDR_0_5};
    uint8_t destMacAddr[kMacSize] = {MAC_ADDR_1_0, MAC_ADDR_1_1, MAC_ADDR_1_2, MAC_ADDR_1_3, MAC_ADDR_1_4, MAC_ADDR_1_5};

    volatile adi_eth_LinkStatus_e    linkStatus;


public:
    bool    begin                   (uint8_t * mac, uint8_t cs_pin = DEFAULT_ETH_SPI_CS_Pin);
    bool    begin                   (uint8_t * mac, uint8_t status, uint8_t interrupt, uint8_t reset, uint8_t chip_select);
    
    bool    sendData                (uint8_t *data, uint16_t dataLen, uint8_t *destMac);
    bool    sendData                (uint8_t *data, uint16_t dataLen);
    uint16_t getRxData              (uint8_t *data, uint16_t dataLen, uint8_t *senderMac);
    bool    getRxAvailable          ();

    void setMac                     (uint8_t * mac);
    void getMac                     (uint8_t * mac);
    void setDestMac                 (uint8_t * mac);
    bool indenticalMacs             (uint8_t * mac1, uint8_t * mac2);

    void setRxCallback              (void (*cbFunc)(uint8_t *, uint16_t, uint8_t*));
    void setLinkCallback            (void (*cbFunc)(bool));
    bool getLinkStatus              (void);

    //User callbacks
    void (*userRxCallback)(uint8_t * data, uint16_t dataLen, uint8_t *senderMac);
    void (*userLinkCallback)(bool connected);
    
    //static functions available to pass to underlying C driver, will regain context and call appropriate member function 
    static void         txCallback_C_Compatible (void *pCBParam, uint32_t Event, void *pArg);
    static void         rxCallback_C_Compatible (void *pCBParam, uint32_t Event, void *pArg);
    static void         linkCallback_C_Compatible(void *pCBParam, uint32_t Event, void *pArg);

    //functions called be driver on successful rx, tx, or link status chage.
    void                txCallback              (void *pCBParam, uint32_t Event, void *pArg);
    void                rxCallback              (void *pCBParam, uint32_t Event, void *pArg);
    void                linkCallback            (void *pCBParam, uint32_t Event, void *pArg);
};

#endif
