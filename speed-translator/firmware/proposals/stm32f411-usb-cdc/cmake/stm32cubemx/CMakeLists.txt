cmake_minimum_required(VERSION 3.22)

# Use the same SDK release as the CubeMX project.
set(STM32CUBEF4_VERSION "1.28.3")
set(STM32CUBEF4_ROOT "" CACHE PATH "Path to the local STM32CubeF4 package")
if(NOT STM32CUBEF4_ROOT OR NOT EXISTS "${STM32CUBEF4_ROOT}/package.xml")
    message(FATAL_ERROR
        "Install STM32CubeF4 ${STM32CUBEF4_VERSION} and set STM32CUBEF4_ROOT "
        "to the package directory containing package.xml. See README.md.")
endif()

file(READ "${STM32CUBEF4_ROOT}/package.xml" STM32CUBEF4_PACKAGE)
string(REPLACE "." "[.]" STM32CUBEF4_VERSION_PATTERN "${STM32CUBEF4_VERSION}")
if(NOT STM32CUBEF4_PACKAGE MATCHES "Patch=\"FW[.]F4[.]${STM32CUBEF4_VERSION_PATTERN}\"")
    message(FATAL_ERROR
        "STM32CUBEF4_ROOT must contain STM32CubeF4 ${STM32CUBEF4_VERSION}. "
        "The package at ${STM32CUBEF4_ROOT} reports a different version.")
endif()

# Enable CMake support for ASM and C languages
enable_language(C ASM)
# STM32CubeMX generated symbols (macros)
set(MX_Defines_Syms 
	USE_HAL_DRIVER 
	STM32F411xE
    $<$<CONFIG:Debug>:DEBUG>
)

# STM32CubeMX generated include paths
set(MX_Include_Dirs
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Inc
    "${STM32CUBEF4_ROOT}/Drivers/STM32F4xx_HAL_Driver/Inc"
    "${STM32CUBEF4_ROOT}/Drivers/STM32F4xx_HAL_Driver/Inc/Legacy"
    "${STM32CUBEF4_ROOT}/Middlewares/ST/STM32_USB_Device_Library/Core/Inc"
    "${STM32CUBEF4_ROOT}/Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc"
    "${STM32CUBEF4_ROOT}/Drivers/CMSIS/Device/ST/STM32F4xx/Include"
    "${STM32CUBEF4_ROOT}/Drivers/CMSIS/Include"
)

# STM32CubeMX generated application sources
set(MX_Application_Src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Src/main.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Src/usb_device.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Src/usbd_conf.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Src/usbd_desc.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Src/usbd_cdc_if.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Src/stm32f4xx_it.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Src/stm32f4xx_hal_msp.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Src/sysmem.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Src/syscalls.c
    ${CMAKE_CURRENT_SOURCE_DIR}/../../startup_stm32f411xe.s
)

# STM32 HAL/LL Drivers
set(STM32_Drivers_Src
    ${CMAKE_CURRENT_SOURCE_DIR}/../../Src/system_stm32f4xx.c
    "${STM32CUBEF4_ROOT}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pcd.c"
    "${STM32CUBEF4_ROOT}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pcd_ex.c"
    "${STM32CUBEF4_ROOT}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_ll_usb.c"
    "${STM32CUBEF4_ROOT}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc.c"
    "${STM32CUBEF4_ROOT}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_rcc_ex.c"
    "${STM32CUBEF4_ROOT}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash.c"
    "${STM32CUBEF4_ROOT}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ex.c"
    "${STM32CUBEF4_ROOT}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_flash_ramfunc.c"
    "${STM32CUBEF4_ROOT}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_gpio.c"
    "${STM32CUBEF4_ROOT}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma_ex.c"
    "${STM32CUBEF4_ROOT}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_dma.c"
    "${STM32CUBEF4_ROOT}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr.c"
    "${STM32CUBEF4_ROOT}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_pwr_ex.c"
    "${STM32CUBEF4_ROOT}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_cortex.c"
    "${STM32CUBEF4_ROOT}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c"
    "${STM32CUBEF4_ROOT}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_exti.c"
    "${STM32CUBEF4_ROOT}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim.c"
    "${STM32CUBEF4_ROOT}/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal_tim_ex.c"
)

# Drivers Midllewares


set(USB_Device_Library_Src
    "${STM32CUBEF4_ROOT}/Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_core.c"
    "${STM32CUBEF4_ROOT}/Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ctlreq.c"
    "${STM32CUBEF4_ROOT}/Middlewares/ST/STM32_USB_Device_Library/Core/Src/usbd_ioreq.c"
    "${STM32CUBEF4_ROOT}/Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Src/usbd_cdc.c"
)

# Link directories setup
set(MX_LINK_DIRS

)
# Project static libraries
set(MX_LINK_LIBS 
    STM32_Drivers
    ${TOOLCHAIN_LINK_LIBRARIES}
    USB_Device_Library	
)
# Interface library for includes and symbols
add_library(stm32cubemx INTERFACE)
target_include_directories(stm32cubemx INTERFACE ${MX_Include_Dirs})
target_compile_definitions(stm32cubemx INTERFACE ${MX_Defines_Syms})

# Create STM32_Drivers static library
add_library(STM32_Drivers OBJECT)
target_sources(STM32_Drivers PRIVATE ${STM32_Drivers_Src})
target_link_libraries(STM32_Drivers PUBLIC stm32cubemx)


# Create USB_Device_Library static library
add_library(USB_Device_Library OBJECT)
target_sources(USB_Device_Library PRIVATE ${USB_Device_Library_Src})
target_link_libraries(USB_Device_Library PUBLIC stm32cubemx)

# Add STM32CubeMX generated application sources to the project
target_sources(${CMAKE_PROJECT_NAME} PRIVATE ${MX_Application_Src})

# Link directories setup
target_link_directories(${CMAKE_PROJECT_NAME} PRIVATE ${MX_LINK_DIRS})

# Add libraries to the project
target_link_libraries(${CMAKE_PROJECT_NAME} ${MX_LINK_LIBS})

# Add the map file to the list of files to be removed with 'clean' target
set_target_properties(${CMAKE_PROJECT_NAME} PROPERTIES ADDITIONAL_CLEAN_FILES ${CMAKE_PROJECT_NAME}.map)

# Validate that STM32CubeMX code is compatible with C standard
if((CMAKE_C_STANDARD EQUAL 90) OR (CMAKE_C_STANDARD EQUAL 99))
    message(ERROR "Generated code requires C11 or higher")
endif()
