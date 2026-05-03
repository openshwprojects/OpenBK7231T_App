# This file is to be given as "make USER_C_MODULES=..." when building Micropython port

add_library(usermod_esp_display_panel INTERFACE)

# Find all source files in the `src` directory.
set(SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/src)
file(GLOB_RECURSE SRCS_C ${SRC_DIR}/*.c)
file(GLOB_RECURSE SRCS_CXX ${SRC_DIR}/*.cpp)

# Find all source files in the `micropython` directory.
set(MPY_DIR ${CMAKE_CURRENT_LIST_DIR}/mpy_support)
file(GLOB_RECURSE MPY_C ${MPY_DIR}/*.c)
file(GLOB_RECURSE MPY_CXX ${MPY_DIR}/*.cpp)

# Add source files to the library.
target_sources(usermod_esp_display_panel INTERFACE ${SRCS_C} ${SRCS_CXX} ${MPY_C} ${MPY_CXX})

# Add the current directory as an include directory.
target_include_directories(usermod_esp_display_panel INTERFACE ${SRC_DIR} ${MPY_DIR})

# Add compile options. Since the target is not created by `idf_component_register()`, we need to add the `ESP_PLATFORM` define manually.
target_compile_options(usermod_esp_display_panel
    INTERFACE
        -Wno-missing-field-initializers -DESP_PLATFORM $<$<COMPILE_LANGUAGE:CXX>:-std=gnu++17>
)

# Link INTERFACE library to the usermod target.
target_link_libraries(usermod INTERFACE usermod_esp_display_panel)
