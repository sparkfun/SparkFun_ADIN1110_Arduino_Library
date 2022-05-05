#ifndef __Sparkfun_SinglePairEth__
#define __Sparkfun_SinglePairEth__

#include "Sparkfun_SinglePairEth_Raw.h"
#include <map>

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

#define kFrameHeaderSize 14

class SinglePairEth : public SinglePairEth_Raw
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

    uint8_t macAddr[2][6] = {
        {MAC_ADDR_0_0, MAC_ADDR_0_1, MAC_ADDR_0_2, MAC_ADDR_0_3, MAC_ADDR_0_4, MAC_ADDR_0_5},
        {MAC_ADDR_1_0, MAC_ADDR_1_1, MAC_ADDR_1_2, MAC_ADDR_1_3, MAC_ADDR_1_4, MAC_ADDR_1_5},
    };
    uint8_t ethFrameHeader[kFrameHeaderSize] =
    {
        MAC_ADDR_0_0, MAC_ADDR_0_1, MAC_ADDR_0_2, MAC_ADDR_0_3, MAC_ADDR_0_4, MAC_ADDR_0_5,
        MAC_ADDR_1_0, MAC_ADDR_1_1, MAC_ADDR_1_2, MAC_ADDR_1_3, MAC_ADDR_1_4, MAC_ADDR_1_5,
        0x08, 0x00,
    };

public:
    adi_eth_Result_e    begin                   (uint8_t cs_pin = DEFAULT_ETH_SPI_CS_Pin);
    adi_eth_Result_e    begin                   (uint8_t status, uint8_t interrupt, uint8_t reset, uint8_t chip_select);
    
    adi_eth_Result_e    sendData                (uint8_t *data, uint16_t dataLen);
    uint16_t            getRxData               (uint8_t *data, uint16_t dataLen);
    bool                getRxAvailable          ();

    adi_eth_Result_e    setRxCallback           (void (*cbFunc)(uint8_t *, uint16_t));
    adi_eth_Result_e    setTxCallback           (void (*cbFunc)());
    adi_eth_Result_e    setLinkCallback         (void (*cbFunc)(adi_eth_LinkStatus_e));

    void (*userRxCallback)(uint8_t * data, uint16_t dataLen);
    void (*userTxCallback)();
    void (*userLinkCallback)(adi_eth_LinkStatus_e);
    volatile adi_eth_LinkStatus_e    linkStatus;

    void                txCallback              (void *pCBParam, uint32_t Event, void *pArg);
    void                rxCallback              (void *pCBParam, uint32_t Event, void *pArg);
    void                linkCallback            (void *pCBParam, uint32_t Event, void *pArg);

    static std::map<adin1110_DeviceHandle_t, SinglePairEth *> devices;
    static void         txCallback_C_Compatible (void *pCBParam, uint32_t Event, void *pArg);
    static void         rxCallback_C_Compatible (void *pCBParam, uint32_t Event, void *pArg);
    static void         linkCallback_C_Compatible(void *pCBParam, uint32_t Event, void *pArg);
};

#endif
