project(transmitter)

add_executable(${PROJECT_NAME}
    transmitter.c
)

target_link_libraries(${PROJECT_NAME}
    pico_stdlib
    hardware_uart
    hardware_spi
    hardware_irq
    driver
)

pico_add_extra_outputs(${PROJECT_NAME})
