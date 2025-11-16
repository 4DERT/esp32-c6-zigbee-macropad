
# --- File & Path Definitions ---
set(ZCL_UTIL_PATH "/opt/esp-idf/examples/zigbee/common/zcl_utility")

set(COMPONENT_SRCS 
    "main.c"
    "macropad.c"
    "macropad_zigbee.c"
    "${ZCL_UTIL_PATH}/src/zcl_utility.c"
)

# Define include directories
set(COMPONENT_INCLUDE_DIRS
    "."
    "${ZCL_UTIL_PATH}/include"
)

# Define COMMON component dependencies
set(COMPONENT_REQUIRES
    keys
    zigbee_core
    nvs_flash
)

# --- Conditional Logic for LED Driver ---
if(CONFIG_MACROPAD_LED_DRIVER_WS2812)
    # Add the WS2812 implementation file
    list(APPEND COMPONENT_SRCS "macropad_led_addressable.c")

    # Add dependencies required ONLY by this driver
    list(APPEND COMPONENT_REQUIRES "ledstrip_fx")
    list(APPEND COMPONENT_REQUIRES "led_strip")

    # WORKAROUND:
    # The ESP-IDF build system is failing to automatically propagate the
    # include path from the 'ledstrip_fx' component. Even though 'ledstrip_fx'
    # is correctly listed in this component's dependencies (REQUIRES/PRIV_REQUIRES)
    # and the 'ledstrip_fx' component itself defines its 'INCLUDE_DIRS'
    # correctly, the compiler still fails with "lsfx.h: No such file or directory".
    # This line manually injects the path into this component's include list
    # to force the compiler to find the headers.
    list(APPEND COMPONENT_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/components/ledstrip_fx/include")

elseif(CONFIG_MACROPAD_LED_DRIVER_GPIO)
    # Add the GPIO implementation file
    list(APPEND COMPONENT_SRCS "macropad_led_gpio.c")

    # Add dependencies required ONLY by this driver
    list(APPEND COMPONENT_REQUIRES "esp_driver_gpio")
else()
    message(WARNING "No LED driver selected in Kconfig!")
endif()

# --- Component Registration ---
# Register the component with all collected sources, includes, and dependencies
idf_component_register(
    SRCS ${COMPONENT_SRCS}
    INCLUDE_DIRS ${COMPONENT_INCLUDE_DIRS}
    REQUIRES ${COMPONENT_REQUIRES}
    PRIV_REQUIRES ${COMPONENT_PRIV_REQUIRES}
)
