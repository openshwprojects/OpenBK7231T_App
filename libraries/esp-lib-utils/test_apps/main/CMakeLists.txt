idf_component_register(
    SRCS "test_app_main.cpp" "test_on_c.c" "test_on_cpp.cpp"
    WHOLE_ARCHIVE
)

idf_build_get_property(build_components BUILD_COMPONENTS)
foreach(COMPONENT ${build_components})
    if(COMPONENT MATCHES "esp-lib-utils" OR COMPONENT MATCHES "espressif__esp-lib-utils")
        set(TARGET_COMPONENT ${COMPONENT})
        break()
    endif()
endforeach()

if(TARGET_COMPONENT STREQUAL "")
    message(FATAL_ERROR "Component 'esp-lib-utils' not found.")
else()
    idf_component_get_property(ESP_LIB_UTILS_LIB ${TARGET_COMPONENT} COMPONENT_LIB)
    set(TARGET_COMPONENT "")
endif()

if(CONFIG_ESP_UTILS_CONF_MEM_GENERAL_ALLOC_TYPE_ESP)
    target_compile_options(
        ${ESP_LIB_UTILS_LIB}
        PUBLIC
            "-DESP_UTILS_CONF_MEM_GENERAL_ALLOC_ESP_CAPS=(MALLOC_CAP_DEFAULT | MALLOC_CAP_8BIT)"
    )
elseif(CONFIG_ESP_UTILS_CONF_MEM_GENERAL_ALLOC_TYPE_CUSTOM)
    target_compile_options(
        ${ESP_LIB_UTILS_LIB}
        PUBLIC
            "-DESP_UTILS_CONF_MEM_GENERAL_ALLOC_CUSTOM_INCLUDE=\"esp_heap_caps.h\""
    )
    target_compile_options(
        ${ESP_LIB_UTILS_LIB}
        PUBLIC
            "-DESP_UTILS_CONF_MEM_GENERAL_ALLOC_CUSTOM_MALLOC(x)=heap_caps_aligned_alloc(1, x, MALLOC_CAP_DEFAULT | MALLOC_CAP_8BIT)"
    )
    target_compile_options(
        ${ESP_LIB_UTILS_LIB}
        PUBLIC
            "-DESP_UTILS_CONF_MEM_GENERAL_ALLOC_CUSTOM_FREE(x)=heap_caps_free(x)"
    )
endif()
