# SIMCom A7680C HTTP handle for python and C(STM32 HAL) updated.
Http method and functions for SIMCom communicate.
# 1. Python + RP4020.
To set up the SIMCom connect 5V and GND to the SIMCom, connect the SIMCom tx rx to the pi pico rx tx
begin the SIMCom connection by 
- import SIMCom
- from machine import UART
- SIM_UART = UART(UART_PORT, BAURATE: 115200, tx = TX_PIN, rx = RX_PIN)
- SIMCom = SIMCom(SIM_UART, PHONE_NUM)

when you want to use the function just do
- SIMCom.handShake()

#Note: Read the comments in the code to change the code to make it fit for your use case

#The code already have the trials handle and time out handle, if you want a better code, try to scan for 'OK' str every time sending command to the SIMCom.

SIMCom.MSG() - for message sending to some number PHONE_NUM

SIMCom.handShake() - Check if the SIMCom is ready for MSG or HTTP, also remove the echo message for better check when needed

# 2. C LIB (STM32 HAL) + STM32
- Connection keeping the same as above with the 5V GND and TX RX pin.
- In this we'll use UART2 as the communicate port and put it in EXT mode
- Init SIM data scanning and interupt handle for the UART
- All of the buffer and flag is init in the lib, change if you want, base on your need

//Init UART receive msg
HAL_UARTEx_ReceiveToIdle_IT(&huart2, SIM_buffer, UART_RX_BUFFER_SIZE);
//uart handle function
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size){
  // SIMCom UART Handle
	if (huart->Instance == USART2) {
    SIM_DataValid = true; // Set the flag to indicate valid SIM data
    HAL_UARTEx_ReceiveToIdle_IT(&huart2, SIM_buffer, UART_RX_BUFFER_SIZE);  //Re-init the UART interupt, change UART_RX_BUFFER_SIZE to your need
  }
}

# ALl fucntion have timeout and error code which you can extract out using SIMCOM_Error struct. Post and get echo data will be kept in SIM_data.
# All function are pre_build and still in testing and refinement state, contribute if you want, thank you.




