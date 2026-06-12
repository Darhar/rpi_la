cmake_minimum_required(VERSION 3.10)

project(la01)

set(CMAKE_CXX_STANDARD 17)

option(USERGB111 "Enable RGB111 color mode" OFF)
include_directories(
    src
    src/hal
    src/drivers
)

add_executable(la01
    src/apps/la01/main.cpp

    src/hal/spi.cpp
    src/hal/gpio.cpp

    src/drivers/ili9488.cpp
    src/drivers/cardkb.cpp
    src/drivers/gusman_logic.cpp
    src/gfx/renderer.cpp
    src/gfx/palette.cpp
    src/terminal/screenbuffer.cpp
    src/gfx/font8x16.cpp
    src/logic/logic_view.cpp
    src/logic/logic_events.cpp
    src/logic/pwm_measure.cpp
    src/logic/uart_decode.cpp
    src/logic/protocols/protocol_registry.cpp
    src/logic/capture_file.cpp
    src/logic/export/export_csv.cpp
)

if(USERGB111)
    target_compile_definitions(la01 PRIVATE USERGB111)
endif()

target_link_libraries(la01
    PRIVATE
    gpiod
    util
)
