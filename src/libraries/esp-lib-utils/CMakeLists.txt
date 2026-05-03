set(SRC_DIR ./src)
file(GLOB_RECURSE SRCS ${SRC_DIR}/*.c)

idf_component_register(
    SRCS ${SRCS}
    INCLUDE_DIRS ${SRC_DIR}
)

target_compile_options(${COMPONENT_LIB}
    PUBLIC
        -Wno-missing-field-initializers
    PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-std=gnu++17>
)
