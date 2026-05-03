idf_component_register(
    SRCS "test_app_main.cpp" "test_lvgl_port.cpp" "lvgl_v8_port.cpp"
    WHOLE_ARCHIVE
)

target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-missing-field-initializers)

# The following code is to avoid the error:
# lvgl_v8_port/managed_components/lvgl__lvgl/demos/stress/lv_demo_stress.c:92:29: error: format '%d' expects argument of
# type 'int', but argument 6 has type 'uint32_t' {aka 'long unsigned int'} [-Werror=format=]

# Get the exact component name
idf_build_get_property(build_components BUILD_COMPONENTS)
foreach(COMPONENT ${build_components})
    if(COMPONENT MATCHES "lvgl" OR COMPONENT MATCHES "lvgl__lvgl")
        set(TARGET_COMPONENT ${COMPONENT})
        break()
    endif()
endforeach()
# Get the component library
if(TARGET_COMPONENT STREQUAL "")
    message(FATAL_ERROR "Component 'lvgl' not found.")
else()
    idf_component_get_property(LVGL_LIB ${TARGET_COMPONENT} COMPONENT_LIB)
endif()
target_compile_options(${LVGL_LIB} PRIVATE "-Wno-format")
set(TARGET_COMPONENT "")
