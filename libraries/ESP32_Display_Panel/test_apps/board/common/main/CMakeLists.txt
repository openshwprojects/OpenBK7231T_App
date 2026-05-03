set(BOARD_CONFIGS_DIR ${CMAKE_CURRENT_LIST_DIR}/board_configs)
file(GLOB_RECURSE BOARD_CONFIGS_SRCS ${BOARD_CONFIGS_DIR}/*.cpp)

idf_component_register(
    SRCS
        "test_app_main.cpp" "test_board_common.cpp" ${BOARD_CONFIGS_SRCS}
    INCLUDE_DIRS
        . ${BOARD_CONFIGS_DIR}
    WHOLE_ARCHIVE
)
