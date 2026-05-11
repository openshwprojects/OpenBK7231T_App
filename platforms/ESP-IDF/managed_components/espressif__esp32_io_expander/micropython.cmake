# This file is to be given as "make USER_C_MODULES=..." when building Micropython port

add_library(usermod_esp_io_expander INTERFACE)

# Set the source directorya and find all source files.
set(SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/src)
file(GLOB_RECURSE SRCS_C ${SRC_DIR}/*.c)
file(GLOB_RECURSE SRCS_CXX ${SRC_DIR}/*.cpp)

# Add our source files to the library.
target_sources(usermod_esp_io_expander INTERFACE ${SRCS_C} ${SRCS_CXX})

# Add the current directory as an include directory.
target_include_directories(usermod_esp_io_expander INTERFACE ${SRC_DIR})

# Add compile options. Since the target is not created by `idf_component_register()`, we need to add the `ESP_PLATFORM` define manually.
target_compile_options(usermod_esp_io_expander
    INTERFACE
        -Wno-missing-field-initializers -DESP_PLATFORM $<$<COMPILE_LANGUAGE:CXX>:-std=gnu++17>
)

# Link our INTERFACE library to the usermod target.
target_link_libraries(usermod INTERFACE usermod_esp_io_expander)
