idf_component_register(
    SRCS "test_app_main.cpp" "test_i2c_touch.cpp"
    WHOLE_ARCHIVE
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-missing-field-initializers)
