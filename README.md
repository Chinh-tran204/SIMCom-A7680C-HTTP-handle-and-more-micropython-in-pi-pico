# SIMCom A7680C HTTP & SMS Library

Welcome! This repository provides a robust and easy-to-use library for interacting with the **SIMCom A7680C** module. Whether you are using **Python (MicroPython)** on a Pi Pico or **C (STM32 HAL)** on an industrial controller, this library helps you handle HTTP requests and SMS messaging with built-in error handling and state management.

---

## 🚀 Hardware Overview: SIMCom A7680C

The SIMCom A7680C is a compact 4G LTE Cat 1 module, ideal for IoT applications requiring high-speed data and voice/SMS capabilities.

### 🔌 Connection Peripheral Guide

To ensure stable communication, follow the wiring guide below. 

| SIMCom A7680C Pin | MCU Pin (RP2040 / STM32) | Notes |
| :--- | :--- | :--- |
| **VCC** | **5V (External)** | Use a stable power source (2A peak recommended) |
| **GND** | **GND** | Common ground with the MCU |
| **TXD** | **RX** | Connect SIMCom TX to MCU RX |
| **RXD** | **TX** | Connect SIMCom RX to MCU TX |

> **Note:** The A7680C typically uses 3.3V logic. If you are using a 5V MCU, ensure you use a logic level shifter to protect the module.

---

## 🐍 1. Python Implementation (MicroPython/RP2040)

Perfect for rapid prototyping on boards like the Raspberry Pi Pico.

### Setup
1. Upload `SIMCom.py` to your MicroPython device.
2. Initialize the UART and the SIMCom class.

### Usage Example
```python
from machine import UART, Pin
import SIMCom

# Initialize UART
# Adjust TX/RX pins according to your board layout
SIM_UART = UART(0, baudrate=115200, tx=Pin(0), rx=Pin(1))

# Initialize Library
# PHONE_NUM is the default recipient for SMS
sim = SIMCom.SIMCom(SIM_UART, "+84123456789")

# 1. Perform Handshake (Initialize AT communication)
if sim.handShake() == 0:
    print("SIMCom Ready!")

# 2. Send an SMS
sim.MSG("Hello from SIMCom A7680C!")

# 3. Perform HTTP GET
result = sim.HTTP_GET()
print(f"HTTP GET Result: {result}")
```

---

## ⚙️ 2. C Implementation (STM32 HAL)

Designed for embedded systems using the STM32 HAL framework. The library uses a non-blocking state machine for efficient background operations.

### Configuration in STM32CubeIDE
1. **UART Setup:** Configure a UART peripheral (e.g., `USART2`) as **Asynchronous**, 115200 Baud, 8N1.
2. **Interrupts:** Enable the UART global interrupt in the NVIC settings.
3. **Buffer Definition:** Ensure `UART_RX_BUFFER_SIZE` is defined in your `main.h`.

### Implementation
Add the following to your `main.c` to handle the UART data stream:

```c
// Initialize UART receive to Idle interrupt
HAL_UARTEx_ReceiveToIdle_IT(&huart2, SIM_buffer, UART_RX_BUFFER_SIZE);

// UART Rx Event Callback
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {
    if (huart->Instance == USART2) {
        SIM_DataValid = true; // Signal that new data is available
        // Re-initialize for the next packet
        HAL_UARTEx_ReceiveToIdle_IT(&huart2, SIM_buffer, UART_RX_BUFFER_SIZE);
    }
}
```

### Usage Example
```c
#include "SIMCOM.h"

void App_Loop() {
    // Perform a POST request
    const char* json_payload = "{\"sensor\": \"temp\", \"value\": 25.5}";
    const char* api_url = "https://your-api.com/data";
    
    SIMCOM_Error err = SIMCom_Post(json_payload, api_url, 10000);
    
    if (err == DONE) {
        // Data in SIM_data buffer
    }
}
```

---

## 🛠️ Key Features
- **Auto-Handshake:** Automatically manages echo removal (`ATE0`) and heartbeats.
- **Error Handling:** Detailed error codes for HTTP stages (URL, Auth, Content, etc.).
- **SNI Support:** Includes `AT+CSSLCFG` configuration for modern HTTPS requirements.
- **Timeout Management:** Integrated timeout logic to prevent system hangs.

## 🤝 Contributing
This library is under active development. If you find a bug or have a feature request, feel free to open an issue or submit a pull request!

---
*Maintained by Chinhtran - Built for the SIMCom A7680C Series.*
