# SIMCom-A7680C-HTTP-handle-and-more-micropython-in-pi-pico
Http method and functions for SIMCom communicate.
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
