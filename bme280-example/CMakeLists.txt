cmake_minimum_required(VERSION 3.22)

# Set toolchain BEFORE project() call
if(NOT CMAKE_TOOLCHAIN_FILE)
    set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/cmake/gcc-arm-none-eabi.cmake")
endif()

#
# This file is generated only once,
# and is not re-generated if converter is called multiple times.
#
# User is free to modify the file as much as necessary
#

# Setup compiler settings
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS ON)


# Define the build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Debug")
endif()

# Path to the local EPD library
set(EPD_LIB_DIR "${CMAKE_SOURCE_DIR}/../EPD")

# Set the project name
set(CMAKE_PROJECT_NAME bme280)

# Enable compile command to ease indexing with e.g. clangd
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

# Core project settings
project(${CMAKE_PROJECT_NAME})
message("Build type: " ${CMAKE_BUILD_TYPE})

# Enable CMake support for ASM and C languages
enable_language(C ASM)

# Create an executable object type
add_executable(${CMAKE_PROJECT_NAME})

# Add STM32CubeMX generated sources
add_subdirectory(cmake/stm32cubemx)

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined library search paths
)

# Add sources to executable
target_sources(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user sources here
    ${CMAKE_SOURCE_DIR}/../SENSORS/bme280.c
    ${CMAKE_SOURCE_DIR}/../SENSORS/mpu6050.c
    ${CMAKE_SOURCE_DIR}/../SENSORS/veml7700.c
    ${EPD_LIB_DIR}/Config/DEV_Config.c
    ${EPD_LIB_DIR}/EPD_2in9_V2.c
    ${EPD_LIB_DIR}/GUI/GUI_Paint.c
    ${EPD_LIB_DIR}/minimal_display.c
    ${EPD_LIB_DIR}/Fonts/font8.c
    ${EPD_LIB_DIR}/Fonts/font12.c
    ${EPD_LIB_DIR}/Fonts/font12CN.c
    ${EPD_LIB_DIR}/Fonts/font16.c
    ${EPD_LIB_DIR}/Fonts/font20.c
    ${EPD_LIB_DIR}/Fonts/font24.c
    ${EPD_LIB_DIR}/Fonts/font24CN.c
)

# Add include paths
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined include paths
    Core/Inc
    ${CMAKE_SOURCE_DIR}/../SENSORS
    ${EPD_LIB_DIR}
    ${EPD_LIB_DIR}/Config
    ${EPD_LIB_DIR}/GUI
    ${EPD_LIB_DIR}/Fonts
)

# Add project symbols (macros)
target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE
    # Add user defined symbols
)

# Remove wrong libob.a library dependency when using cpp files
list(REMOVE_ITEM CMAKE_C_IMPLICIT_LINK_LIBRARIES ob)

# Add linked libraries
target_link_libraries(${CMAKE_PROJECT_NAME}
    stm32cubemx

    # Add user defined libraries
)
