add_executable(ampel
        main.c
)

target_link_libraries(ampel pico_stdlib freertos)
pico_add_extra_outputs(ampel)
pico_enable_stdio_usb(ampel 1)
