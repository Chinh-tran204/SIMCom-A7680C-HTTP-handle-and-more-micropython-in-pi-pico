///Include
#include "SIMCOM.h"

#define GET 0
#define POST 1
#define PWD 2
#define WUP 3

typedef enum {
	AT,
    CSSLCFG,
    HTTPINIT,
    HTTPPARA_URL,
    HTTPPARA_CONTENT,
    HTTPPARA_AUTH,
    HTTPDATA_LEN,
    TRANSMIT_DATA,
    HTTPACTION,
    HTTPREAD,
    HTTPTERM_1,
    HTTPTERM_2,
	SLEEP,
	WAKEUP,
//    RESULT,
    AWAIT
} SIMCOM_State;

typedef enum {
    CMD_AT_idx,
    CMD_HTTP_TERM_idx,
    CMD_enableSNI_idx,
    CMD_HTTP_INIT_idx,
    CMD_PARA_CONTENT_idx,
    CMD_HTTP_POST_idx,
    CMD_HTTP_GET_idx,
    CMD_HTTP_READ_idx,
    CMD_SLEEP_idx,
    CMD_WAKE_idx
} SIMCOM_Cmd;

extern UART_HandleTypeDef huart2;
UART_HandleTypeDef *p_sim_uart = &huart2;

void change_uart_port(UART_HandleTypeDef *uart_port){
    p_sim_uart = uart_port;
}

//SIMCom Flag
volatile bool SIM_DataValid = false;
/* SIMCom UART buffer and data container */
// For data manipulating
uint8_t SIM_data[UART_RX_BUFFER_SIZE];
// For storing uart buffer and commute
uint8_t SIM_buffer[UART_RX_BUFFER_SIZE];

/* Operation variable */
static uint8_t retrials = 0U;
static uint32_t endTime = 0U;

static char char_url[200];
static char author[200];

const char *messeage[] = {
	//Post
    "AT\r",
	"AT+HTTPTERM\r",
	"AT+CSSLCFG=\"enableSNI\",0,1\r",
	"AT+HTTPINIT\r",
	"AT+HTTPPARA=\"CONTENT\",\"application/json\"\r",
	"AT+HTTPACTION=1\r",
    "AT+HTTPACTION=0\r",
	"AT+HTTPREAD=0,300\r",
	//Sleep mode
	"AT+CSCLK=2\r",
	"WAKEUP\r"
};

static SIMCOM_Cmd   CMD = CMD_AT_idx;
static SIMCOM_State STATE = HTTPTERM_1;
static SIMCOM_State PRE_STATE = HTTPTERM_1;
static SIMCOM_Error Result = DONE;


//Delay
void delay_ms(uint32_t delayTime){
	uint32_t startTime = HAL_GetTick();
	while((HAL_GetTick() - startTime <= delayTime)){}
}

/* Time out function; true when the time is come */
bool timeOutHandle(uint32_t ext_time){
    if(Result != EMPTY){
        /* reset time out flag and range */
        endTime = HAL_GetTick() + ext_time;
        /* reset state for next operation */
        STATE = HTTPTERM_1;
        PRE_STATE = STATE;
        Result = EMPTY;
        retrials = 0;
        return false;
    } else if(HAL_GetTick() >= endTime){
        return true;
    } else {
        return false;
    }
}


// //Send data
void Transmit(const char *cmd){
    if(STATE != PRE_STATE){     //resset retrials if state change
        PRE_STATE = STATE;
        retrials = 0;
    }
 	HAL_UART_Transmit(p_sim_uart, (uint8_t*)cmd, strlen(cmd), 500);// Send data UART
// 	memset(SIM_buffer,'\0',UART_RX_BUFFER_SIZE);		//reset receive buffer
// 	LOG("[SIM_CMD]",cmd);
    STATE = AWAIT;
}

float SIMCom_HandShake(void){
	Transmit("AT\r");
	delay_ms(20);
	if (!(strstr((char *)SIM_buffer, "OK"))){
		return -1.0f;
	} else return 0.0f;
}

void SIM_CheckResponse(uint8_t action){
    if(!SIM_DataValid) {
    	return;
    }
    /* Reset the UART flag after processing */
	SIM_DataValid = false; // 
    switch (PRE_STATE) {
        case AT:
            if(retrials > 5){
                Result = ERR_HANDSHAKE;
            } else if (strstr((char *)SIM_buffer, "OK")) {
            	if(action == 2) STATE = SLEEP;
            	else if(action == 3) Result = DONE;
            	else STATE = CSSLCFG;
            } else {
                retrials++;
            }
            break;
        case CSSLCFG:
            if(retrials > 5){
                Result = ERR_CSSLCFG;
            } else if (strstr((char *)SIM_buffer, "OK")) {
                STATE = HTTPINIT;
            } else {
                retrials++;
            }
            break;
        case HTTPINIT:
            if(retrials > 5){
                Result = ERR_HTTPINIT;
            } else if (strstr((char *)SIM_buffer, "OK")) {
                STATE = HTTPPARA_URL;
            } else {
                retrials++;
            }
            break;
        case HTTPPARA_URL:
            if(retrials > 5){
                Result = ERR_URL;
            } else if (strstr((char *)SIM_buffer, "OK")) {
            	if(action) STATE = HTTPPARA_CONTENT;
            	else STATE = HTTPPARA_AUTH;
            } else {
                retrials++;
            }
            break;
        case HTTPPARA_CONTENT:
            if(retrials > 5){
                Result = ERR_CONTENT;
            } else if (strstr((char *)SIM_buffer, "OK")) {
                STATE = HTTPDATA_LEN; //////test
            } else {
                retrials++;
            }
            break;
        case HTTPDATA_LEN:
            if(retrials > 5){
                Result = ERR_DATA_LEN;
            } else if (strstr((char *)SIM_buffer, "DOWNLOAD")) {
                STATE = TRANSMIT_DATA;
            } else {
                retrials++;
            }
            break;
        case TRANSMIT_DATA:
            if(retrials > 5){
                Result = ERR_TRANSMIT_DATA;
            } else if (strstr((char *)SIM_buffer, "OK")) {
                STATE = HTTPACTION;
            } else {
                retrials++;
            }
            break;
        case HTTPACTION:
			if(retrials > 5){
				Result = ERR_ACTION;
			} else if(action){
					if (strstr((char *)SIM_buffer, "201") || strstr((char *)SIM_buffer, "400")) {
						STATE = HTTPREAD;
					} else {
						retrials++;
					}
			} else if (!action) {
					if (strstr((char *)SIM_buffer, "200") || strstr((char *)SIM_buffer, "400")) {
						STATE = HTTPREAD;
					} else {
						retrials++;
					}
			} else {
				retrials++;
			}
			break;
		case HTTPREAD:
			if(retrials > 5){
				Result = ERR_READ;
			} else if(strstr((char *)SIM_buffer, "OK")) { ///success
				memset(SIM_data,'\0',sizeof(SIM_data));
				strcpy((char *)SIM_data, (char *)SIM_buffer); // Copy response to SIM_buffer
				STATE = HTTPTERM_2;
			} else {
				retrials++;
			}
			break;
		case HTTPTERM_1:
            STATE = AT;
            break;
        case HTTPTERM_2:
            if(retrials > 5){
                Result = ERR_TERM;
            } else if (strstr((char *)SIM_buffer, "OK")) {
                Result = DONE;
            } else {
                retrials++;
            }
            break;
        case SLEEP:
            if(retrials > 5){
                Result = ERR_SLEEP;
            } else if (strstr((char *)SIM_buffer, "OK")) {
                STATE = AT;
                Result = DONE;
            } else {
                retrials++;
            }
            break;
        case AWAIT:
        	STATE = AT;
        	PRE_STATE = HTTPTERM_1;
        	break;
        default:
        	break;
    }
    memset(SIM_buffer,'\0',UART_RX_BUFFER_SIZE);		//reset receive buffer
}


/* SIM Post action */
SIMCOM_Error SIMCom_Post(const char* data, const char* url, uint32_t timeOut){
    char data_len_buffer[100];
    while(Result == EMPTY ) {
        /* Timeout handle */
        if(timeOutHandle(timeOut)){
            Result = ERR_UART;
            break;
        }
        /* Buffer time for safe operation */
        delay_ms(50);
        switch (STATE) {
            case AT:
                Transmit(messeage[CMD = CMD_AT_idx]); //AT
                break;
            case CSSLCFG:
                Transmit(messeage[CMD = CMD_enableSNI_idx]); //AT+CSSLCFG
                break;
            case HTTPINIT:
                Transmit(messeage[CMD = CMD_HTTP_INIT_idx]); //AT+INIT
                break;
            case HTTPPARA_URL:
                snprintf(char_url, sizeof(char_url), "AT+HTTPPARA=\"URL\",\"%s\"\r", url);
                Transmit(char_url); //AT+HTTPPARA URL
                break;
            case HTTPPARA_CONTENT:
                Transmit(messeage[CMD = CMD_PARA_CONTENT_idx]); //AT+HTTPPARA CONTENT
                break;
            case HTTPPARA_AUTH:
                break;
            case HTTPDATA_LEN:
                snprintf(data_len_buffer, sizeof(data_len_buffer), "AT+HTTPDATA=%d,10000\r", strlen(data));
                Transmit(data_len_buffer); //AT+HTTPDATA LEN
                break;
            case TRANSMIT_DATA:
                Transmit(data);         //Transmit DATA
                break;
            case HTTPACTION:
                Transmit(messeage[CMD = CMD_HTTP_POST_idx]);   //AT+HTTPACTION - POST
                break;
            case HTTPREAD:
                Transmit(messeage[CMD = CMD_HTTP_READ_idx]);   //AT+HTTPREAD
                break;
            case HTTPTERM_1:
                Transmit(messeage[CMD = CMD_HTTP_TERM_idx]);   //AT+HTTPTERM
                break;
            case HTTPTERM_2:
                Transmit(messeage[CMD = CMD_HTTP_TERM_idx]);   //AT+HTTPTERM
                break;
            case AWAIT:
                SIM_CheckResponse(POST);
                break;
            default:
                break;
        }
    }
    // Return the result
    delay_ms(1000);
    return Result;
}

SIMCOM_Error SIMCom_Get(const char *id_machine, const char* url, uint32_t timeOut){
    while (Result == EMPTY) {
        /* Timeout handle */
        if(timeOutHandle(timeOut)){
            Result = ERR_UART;
            break;
        }
        /* Buffer time for safe operation */
        delay_ms(50);
        switch (STATE) {
            case AT:
                Transmit(messeage[CMD = CMD_AT_idx]); //AT
                break;
            case CSSLCFG:
                Transmit(messeage[CMD = CMD_enableSNI_idx]); //AT+CSSLCFG
                break;
            case HTTPINIT:
                Transmit(messeage[CMD = CMD_HTTP_INIT_idx]); //AT+INIT
                break;
            case HTTPPARA_URL:
                snprintf(char_url, sizeof(char_url), "AT+HTTPPARA=\"URL\",\"%s\"\r", url);
                Transmit(char_url); //AT+HTTPPARA URL
                break;
            case HTTPPARA_AUTH:
                snprintf(author, sizeof(author), "AT+HTTPPARA=\"USERDATA\",\"Authorization: Basic %s\"\r", id_machine);
                Transmit(author);       //AT+HTTPPARA AUTH
                break;
            case HTTPACTION:
                Transmit(messeage[CMD = CMD_HTTP_GET_idx]);   //AT+HTTPACTION - GET
                break;
            case HTTPREAD:
                Transmit(messeage[CMD = CMD_HTTP_READ_idx]);   //AT+HTTPREAD
                break;
            case HTTPTERM_1:
                Transmit(messeage[CMD = CMD_HTTP_TERM_idx]);   //AT+HTTPTERM
                break;
            case HTTPTERM_2:
                Transmit(messeage[CMD = CMD_HTTP_TERM_idx]);   //AT+HTTPTERM
                break;
            case AWAIT:
                SIM_CheckResponse(GET);
                break;
            default:
                STATE = HTTPTERM_1;
                break;
        }
    }
    // Return the result
    delay_ms(1000);
    return Result; 
}

SIMCOM_Error SIM_Sleep(uint32_t timeOut){
    while(Result == EMPTY ) {
        /* Timeout handle */
        if(timeOutHandle(timeOut)){Result = ERR_UART;}
        /* Buffer time for safe operation */
        delay_ms(50);
        switch (STATE) {
            case AT:
                Transmit(messeage[CMD = CMD_AT_idx]); //AT
                break;
            case SLEEP:
                Transmit(messeage[CMD = CMD_SLEEP_idx]); //AT+CSCLK=2
                break;
            case HTTPTERM_1:
                STATE = AT;
                break;
            case AWAIT:
                SIM_CheckResponse(PWD);
                break;
            default:
                break;
        }
    }
    //return the result
    delay_ms(1000);
    return Result;
    
}


SIMCOM_Error SIM_Wakeup(uint32_t timeOut){
    while(Result == EMPTY ) {
        /* Timeout handle */
        if(timeOutHandle(timeOut)){Result = ERR_UART;}
        /* Buffer time for safe operation */
        delay_ms(50);
        switch (STATE) {
            case AT:
                SIM_DataValid = false;
                HAL_UART_Transmit(p_sim_uart, (uint8_t*)"AT\r", 3, 50);// Send data UART
                // 	LOG("[SIM_CMD]",cmd);
                STATE = AWAIT;
                PRE_STATE = AT;
                break;
            case HTTPTERM_1:
                HAL_UART_Transmit(p_sim_uart, (uint8_t*) "AT\r", 3, 50);
                delay_ms(100);
                STATE = AT;
                break;
            case AWAIT:
                SIM_CheckResponse(WUP);
                break;
            default:
                break;
        }
    }
    //return the result
    delay_ms(1000);
    return Result;
}

//uint8_t SIM_Sleep(uint32_t timeOut){
//    if(initTime == 0){
//        initTime = HAL_GetTick();
//    } else if (HAL_GetTick() - initTime >= timeOut){
//        Result = ERR_UART;
//    }
//    delay_ms(100);
//    switch (STATE) {
//        case AT:
//            Transmit(messeage[0]); //AT
//            break;
//        case SLEEP:
//            Transmit("AT+CSCLK?\r"); //AT+CSCLK?
//            break;
//        /* Adding sleep function and also in the Check Response */
//        case HTTPTERM_1:
//            Transmit(messeage[7]);   //AT+HTTPTERM
//            break;
//        case AWAIT:
//            if(SIM_DataValid){
//                SIM_DataValid = false; // Reset the flag after processing
//                SIM_CheckResponse(0);
//            }
//            break;
//    }
//    if(Result == EMPTY){
//        return SIM_Sleep(timeOut);
//    } else {
//        retrials = 0;
//        initTime = 0;
//        STATE = HTTPTERM_1;
//        uint8_t tempResult = Result;
//        Result = EMPTY;
//        return tempResult;
//    }
//}
/* TO DO LIST
* set SIM_DataValid in main to false when begin running SIM_GET or anything
* in main when try to run SIM function init a counter for time it run
*/
