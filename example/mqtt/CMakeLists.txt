cmake_minimum_required(VERSION 3.30)
project(a7672sConnect C)

set(CMAKE_C_STANDARD_REQUIRED ON)

# Add the executable
# Gather all .c sources from the ritJson and example folders
file(GLOB A7672S_CONNECT_SRC "${CMAKE_CURRENT_SOURCE_DIR}/../../a7672sConnect/a7672sConnect.c")
file(GLOB MQTT_EXAMPLE_SRC "${CMAKE_CURRENT_SOURCE_DIR}/a7672sConnect_mqtt.c")
set(SRC_FILES ${A7672S_CONNECT_SRC} ${MQTT_EXAMPLE_SRC})

# include paths (headers live next to a7672sConnect.c)
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/../../a7672sConnect")

# executable for the MQTT example
add_executable(a7672sConnect_mqtt ${SRC_FILES})