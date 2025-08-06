# Generated CMake Pico project file

cmake_minimum_required(VERSION 3.15)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 13_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================

set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(main VERSION 0.1)

# Pull in FreeRTOS
include($ENV{FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable (use main.c ou outro nome que preferir)
add_executable(main
    main.c
    ssd1306.c
    display_oled.c
)

# pull in common dependencies
target_link_libraries(main
    pico_stdlib
    FreeRTOS-Kernel
    FreeRTOS-Kernel-Heap4
    hardware_adc
    hardware_gpio
    hardware_uart
    hardware_i2c
    hardware_spi
)

# include diretórios do projeto, se houver headers no mesmo diretório
target_include_directories(main PRIVATE ${CMAKE_CURRENT_LIST_DIR})

# Habilita printf via USB (1 = ativado) e UART (0 = desativado)
pico_enable_stdio_uart(main 0)
pico_enable_stdio_usb(main 1)

# Gera arquivos extra: .uf2, .bin, .hex, .map etc.
pico_add_extra_outputs(main)
