# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

cmake_minimum_required(VERSION 3.12)

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(pico_sdk_import.cmake)

project(pico_freertos_samples)

# initialize the Pico SDK
pico_sdk_init()

# rest of your project

add_subdirectory(freertos)

add_executable(pico_freertos_samples
        main.c
        SSD1306/lib/ssd1306.c
        SSD1306/lib/twi.c
)

target_link_libraries(pico_freertos_samples 
            pico_stdlib 
            freertos
            hardware_i2c
            )
pico_add_extra_outputs(pico_freertos_samples)
pico_enable_stdio_usb(pico_freertos_samples 1)

