#include "stm32f1xx_hal.h"
#include "main.h"
#include <stdint.h>
#include <stdbool.h>
#include <string.h>


#ifndef INC_SIMCOM_H_
#define INC_SIMCOM_H_

extern volatile bool SIM_DataValid;
extern uint8_t SIM_data[];
extern uint8_t SIM_buffer[];

typedef enum {
    DONE,
    ERR_HANDSHAKE,
    ERR_CSSLCFG,
    ERR_HTTPINIT,
    ERR_URL,
    ERR_CONTENT,
    ERR_AUTH,
    ERR_DATA_LEN,
    ERR_TRANSMIT_DATA,
    ERR_ACTION,
    ERR_READ,
    ERR_TERM,
    ERR_UART,
	ERR_SLEEP,
    EMPTY
} SIMCOM_Error;

uint8_t SIMCom_Init(void);//Create interrupt
float SIMCom_HandShake(void); //handShake
SIMCOM_Error SIMCom_Post(const char* data, const char* id_machine, uint32_t timeOut); //post method
SIMCOM_Error SIMCom_Get(const char* id_machine, uint32_t timeOut); //get method

SIMCOM_Error SIM_Sleep(uint32_t timeOut);
SIMCOM_Error SIM_Wakeup(uint32_t timeOut);
#endif /* INC_SIMCOM_H_ */
