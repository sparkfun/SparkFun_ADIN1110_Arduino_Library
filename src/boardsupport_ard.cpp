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

#include "boardsupport.h"

#include <string.h>

#include "Arduino.h"
#include <SPI.h>

// #define DEBUG_SPI

#if defined(ARDUINO_ARCH_MBED) 
#include <Semaphore.h>
//defines for mbed workaround for IRQ callback
rtos::Thread thread;
// volatile int32_t updates = 0;
rtos::Semaphore updates(0);
#endif

#define RESET_DELAY       (10)
#define AFTER_RESET_DELAY (200)

static          ADI_CB gpfSpiCallback = NULL;
static void     *gpSpiCBParam = NULL;

static          ADI_CB gpfGPIOIntCallback = NULL;
static void     *gpGPIOIntCBParam = NULL;

static uint8_t status_led_pin = LED_BUILTIN;
static uint8_t interrupt_pin = DEFAULT_ETH_INT_Pin;
static uint8_t reset_pin = DEFAULT_ETH_RESET_Pin;
static uint8_t chip_select_pin = DEFAULT_ETH_SPI_CS_Pin;

static SPIClass* SPI_instance = &SPI;

void SPI_TxRxCpltCallback(void);
void BSP_IRQCallback(void);

void BSP_disableInterrupts(void)
{
    #if !defined(ARDUINO_APOLLO3_SFE_ARTEMIS_MM_PB)
        noInterrupts();
    #endif // !defined(ARDUINO_APOLLO3_SFE_ARTEMIS_MM_PB)
}


void BSP_enableInterrupts(void)
{
    #if !defined(ARDUINO_APOLLO3_SFE_ARTEMIS_MM_PB)
        noInterrupts();
    #endif // !defined(ARDUINO_APOLLO3_SFE_ARTEMIS_MM_PB)
}
/*
 * @brief Blocking delay function
 *
 * @param [in]      delay - delay in miliseconds
 * @param [out]     none
 * @return none
 *
 * @details Based on assumption that SysTick counter fires every milisecond
 *
 * @sa
 */
void BSP_delayMs(uint32_t delay)
{
    volatile uint32_t now;
    uint32_t checkTime  = BSP_SysNow();
    /* Read SysTick Timer every Ms*/
    while (1)
    {
      now  = BSP_SysNow();
       if (now - checkTime >= delay)
       {
          break;
       }
    }
}

/*
 * @brief Hardware reset to DUT
 *
 * @param [in]      set - obosolete, keep it for consistancy with older versions
 * @param [out]     none
 * @return none
 *
 * @details Puld down Reset Pin, wait for 1mS release the Reset Pin
 *
 * @sa
 */
void BSP_HWReset(bool set)
{
    digitalWrite(reset_pin, LOW);
    BSP_delayMs(RESET_DELAY);
    digitalWrite(reset_pin, HIGH);
    BSP_delayMs(AFTER_RESET_DELAY);
}

/* LED functions */

static void bspLedSet(uint16_t pin, bool on)
{
    if (on)
    {
        digitalWrite(pin, HIGH);
    }
    else
    {
        digitalWrite(pin, LOW);
    }
}

static void bspLedToggle(uint16_t pin)
{
    digitalWrite(pin, !digitalRead(pin));
}

/*
 * Heartbeat LED, ORANGE
 */
void BSP_HeartBeat(void)
{
    bspLedToggle(status_led_pin);
}

/*
 * HeartBeat LED, RED
 */
void BSP_HeartBeatLed(bool on)
{
    bspLedSet(status_led_pin, on);
}

/*
 * Error LED, RED
 */
void BSP_ErrorLed(bool on)
{
    //NO ERROR LED
    //bspLedSet(BSP_LED2_PORT, BSP_LED2_PIN, on);
}

/*
 * Custom function 1 LED
 */
void BSP_FuncLed1(bool on)
{   
  //NO FuncLed1 LED
  //bspLedSet(BSP_LED1_PORT, BSP_LED1_PIN, on);
}

void BSP_FuncLed1Toggle(void)
{
    //NO FuncLed1 LED
    //bspLedToggle(BSP_LED1_PORT, BSP_LED1_PIN);
}

/*
 * Custom function 2 LED
 */
void BSP_FuncLed2(bool on)
{
    //NO FuncLed2 LED
    //bspLedSet(BSP_LED4_PORT, BSP_LED4_PIN, on);
}

void BSP_FuncLed2Toggle(void)
{
    //NO FuncLed2 LED
    //bspLedToggle(BSP_LED4_PORT, BSP_LED4_PIN);
}

/* All LEDs toggle, used to indicate hardware failure on the board */
void BSP_LedToggleAll(void)
{
    bspLedSet(status_led_pin, HIGH);
}

uint32_t BSP_spi2_write_and_read(uint8_t *pBufferTx, uint8_t *pBufferRx, uint32_t nbBytes, bool useDma)
{
    if(!pBufferTx || !pBufferRx)
    {
        return 1;
    }
    if(useDma)
    {
        return 1;
    }
    #ifdef DEBUG_SPI
    Serial.printf("writing numbytes = %d: ", nbBytes);
    for(int i = 0; i < nbBytes; i++)
    {
        Serial.printf(" %02X", pBufferTx[i]);
    }
    Serial.println();
    #endif
    memcpy(pBufferRx, pBufferTx, nbBytes);
    SPI_instance->beginTransaction(SPISettings(4000000, MSBFIRST, SPI_MODE0));
    bspLedSet(chip_select_pin, LOW);
    SPI_instance->transfer(pBufferRx, nbBytes);
    //Driver expects there to be an interrupt that fires after completion
    //Since SPI is blocking for arduino this is overly complicated, just call the "callback" function now
    SPI_TxRxCpltCallback();
    #ifdef DEBUG_SPI
    Serial.printf("read: ");
    for(int i = 0; i < nbBytes; i++)
    {
        Serial.printf(" %02X", pBufferRx[i]);
    }
    Serial.println();
    #endif
    return 0;
}

// extern uint32_t HAL_SPI_Register_Callback(ADI_CB const *pfCallback, void *const pCBParam);
uint32_t BSP_spi2_register_callback(ADI_CB const *pfCallback, void *const pCBParam)
{
    gpfSpiCallback = (ADI_CB)pfCallback;
    gpSpiCBParam = pCBParam ;
    return 0;
}

void SPI_TxRxCpltCallback(void)
{
    bspLedSet(chip_select_pin, HIGH);
    SPI_instance->endTransaction();
    (*gpfSpiCallback)(gpSpiCBParam, 0, NULL);
}


/*
 * @brief Helper for Access BSP EEPROM, chip select is active high
 *
 * @param [in]      output - true/false
 * @param [out]     none
 * @return pin value
 *
 * @details
 *
 * @sa
 */
void setSPI2Cs(bool set)
{
    if(set == true)
    {
        bspLedSet(chip_select_pin, HIGH);
    }
    else
    {
        bspLedSet(chip_select_pin, LOW);
    }
}


/**
  * @brief  Return the selected Button state.
  * @param  Button: Specifies the Button to be checked.
  *   This parameter should be: BUTTON_USER
  * @retval Button state.
  */

void BSP_getConfigPins(uint16_t *value)
{
    //no buttons
    // uint16_t val16 = 0;
    // uint16_t returnVal = 0;

    // val16 = HAL_GPIO_ReadPin(CFG0_GPIO_Port, CFG0_Pin);

    // returnVal |= val16 << 0;

    // val16 = HAL_GPIO_ReadPin(CFG1_GPIO_Port, CFG1_Pin);

    // returnVal |= val16 << 1;

    // val16 = HAL_GPIO_ReadPin(CFG2_GPIO_Port, CFG2_Pin);

    // returnVal |= val16 << 2;

    // val16 = HAL_GPIO_ReadPin(CFG3_GPIO_Port, CFG3_Pin);

    // returnVal |= val16 << 3;

    // *value = returnVal ;
}

// extern uint32_t HAL_INT_N_Register_Callback(ADI_CB const *pfCallback, void *const pCBParam);
uint32_t BSP_RegisterIRQCallback(ADI_CB const *intCallback, void * hDevice)
{
    gpfGPIOIntCallback = (ADI_CB)intCallback;
    gpGPIOIntCBParam = hDevice ;

    attachInterrupt(interrupt_pin, BSP_IRQCallback, FALLING);
    return 0;
}

#if defined(ARDUINO_ARCH_MBED) 
//MBED will not allow SPI calls during IRQ callback, instead start a thread that will call back after signalled to by irq
int num_int = 0;
void BSP_IRQCallback()
{
    //updates++;
    // num_int++;
    updates.release();
    // if(num_int >2)
    // {
        // Serial.println("Interrupt!");
    // }
}

void thread_fn( void ){  
    while(1)
    {
        updates.acquire();
        // DEBUG_MESSAGE("aquired\n");
        // now this runs on the main thread, and is safe
        // if (v == 1) {
            // updates = 0;
            // DEBUG_MESSAGE("C\n");
            if (gpfGPIOIntCallback)
            {
                (*gpfGPIOIntCallback)(gpGPIOIntCBParam, 0, NULL);
            }
        // }
        // if (updates > 1) {
        //     DEBUG_MESSAGE("CALLBACK troubles!\r\n");
        //     // if (gpfGPIOIntCallback)
        //     // {
        //     //     (*gpfGPIOIntCallback)(gpGPIOIntCBParam, 0, NULL);
        //     // }
        // }
    }
}
#else
void BSP_IRQCallback()
{
    if (gpfGPIOIntCallback)
    {
        (*gpfGPIOIntCallback)(gpGPIOIntCBParam, 0, NULL);
    }
}
#endif

uint32_t msgWrite(char * ptr)
{
    Serial.print(ptr);
    return 0;
}

uint32_t BSP_SysNow(void)
{
    return millis();
}


uint32_t BSP_InitSystem(void)
{
#if defined(ARDUINO_ARCH_MBED) 
    thread.start(thread_fn);
    thread.set_priority(osPriorityHigh);
#endif
    SPI_instance->begin();
    pinMode(status_led_pin, OUTPUT);
    pinMode(interrupt_pin, INPUT);
    pinMode(reset_pin, OUTPUT);
    pinMode(chip_select_pin, OUTPUT);
    digitalWrite(chip_select_pin, HIGH);
    digitalWrite(reset_pin, HIGH);
    return 0;
}

uint32_t BSP_ConfigSystem(uint8_t status, uint8_t interrupt, uint8_t reset, uint8_t chip_select)
{
    status_led_pin = status;
    interrupt_pin = interrupt;
    reset_pin = reset;
    chip_select_pin = chip_select;
    return 0;
}

uint32_t BSP_ConfigSystemCS(uint8_t chip_select)
{
    chip_select_pin = chip_select;
    return 0;
}

char aDebugString[150u];

void common_Fail(char *FailureReason)
{
    char fail[] = "Failed: ";
    char term[] = "\n\r";

    /* Ignore return codes since there's nothing we can do if it fails */
    msgWrite(fail);
    msgWrite(FailureReason);
    msgWrite(term);
 }

void common_Perf(char *InfoString)
{
    char term[] = "\n\r";

    /* Ignore return codes since there's nothing we can do if it fails */
    msgWrite(InfoString);
    msgWrite(term);
}

