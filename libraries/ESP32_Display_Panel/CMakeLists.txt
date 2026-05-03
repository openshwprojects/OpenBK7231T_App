set(SRCS_DIR "src")

file(GLOB_RECURSE CPP_SRCS "${SRCS_DIR}/*.cpp")
file(GLOB_RECURSE C_SRCS "${SRCS_DIR}/*.c")

idf_component_register(
    SRCS ${C_SRCS} ${CPP_SRCS}
    INCLUDE_DIRS ${SRCS_DIR} "."
    REQUIRES driver esp_lcd ESP32_IO_Expander esp-lib-utils
)

target_compile_options(${COMPONENT_LIB}
    PUBLIC
        -Wno-missing-field-initializers
    PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-std=gnu++17>
)

target_compile_definitions(${COMPONENT_LIB}
    PUBLIC
        ESP_PANEL_BOARD_DEFAULT_USE_SUPPORTED=1
        ESP_PANEL_BOARD_USE_SUPPORTED_FILE=1
        ESP_PANEL_BOARD_INCLUDE_SUPPORTED_SIMPLE=1
        BOARD_WAVESHARE_ESP32_S3_TOUCH_LCD_5=1
)
