**Firmware Project README**
Ivars Renge
Created: 2025.06
Goal: Build real time framework, simple, robust, small, but real time
---

## 1. Project Overview

This repository contains firmware for the STM32F3xx series microcontrollers, implementing a lightweight task scheduler, USB console interface, basic filesystem, and hardware abstraction layers (HAL). 
It was developed as a rapid-prototype over a 24‑hour sprint and aims to serve as a starting point for more robust embedded applications.

## 2. Features

* **Preemptive Task Scheduler**: Simple cooperative scheduler with configurable timeouts.

* **USB Console**: Virtual COM port over USB for interactive command and debug I/O.

* **Basic Filesystem**: SPI flash (or internal flash) based FAT-like filesystem stub.

* **HAL Integration**: Wrapper modules for STM32Cube HAL to abstract peripherals:

## 3. Directory Structure

Include 
```
├── Inc/               # Public headers
│   ├── main.h         # Your h
│   ├── user.h         # 
│   └── /           # Peripheral abstraction headers
├── Src/               # Source files
│   ├── main.c         # your c
│   ├── user_tasks.c   # 
│   ├── stmxxxx.c
│   ├── sysxxxx.c
│   └── /
├── Sys/               # Firmware files
│   ├── core.c
│   ├── console.c
│   ├── filesystem.c
│   ├── usb.c
│   └── /
└── Other
```

## 4. Prerequisites

* **Toolchain**: ARM GCC (e.g. `arm-none-eabi-gcc`) or STM32CubeIDE
* **STM32Cube HAL**: Version matching F3 series

## 5. Setup the Firmware

Your main file should:
#include <core.h>

onBoot(); in your initial main.c file

while (1) {
    onLoop();    
}

Your main.h file should contain

#define FIRMWARE_VERSION "Test Application 1.0"

#define USING_USB 1 // if you prefer usb
#define USING_CONSOLE 1 // if you prefer console
#define USING_FILESYSTEM 1 // if you need simple filesystem
#define USING_BUTTONS 1 // if you intend to use button handling



## 6. Flashing & Running


## 7. Usage

* Connect a USB cable to the device.
* Open a serial terminal TeraTerm, look up for virtual com port
* Available commands:

  * `help` — List supported commands

## 8. Configuration & Magic Numbers

All board‑specific parameters (e.g. `TASK_TIMEOUT_MS`, buffer sizes) live in `sys/core.h`. Update this file rather than editing multiple modules directly.

## 9. Error Handling & Logging


## 10. Testing



## 12. License

This project is licensed under the MIT License. 
