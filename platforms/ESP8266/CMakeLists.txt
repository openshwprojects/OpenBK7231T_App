cmake_minimum_required(VERSION 3.13.0)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
idf_build_set_property(COMPILE_OPTIONS "-DPLATFORM_ESP8266=1" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-DLWIP_NOASSERT=1" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-DUSER_SW_VER=\"$ENV{APP_VERSION}\"" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-DOBK_VARIANT=$ENV{OBK_VARIANT}" APPEND)
include_directories("$ENV{IDF_PATH}/../../libraries/berry/src")
include_directories("$ENV{IDF_PATH}/../../include")
set(PROJECT_VER "$ENV{APP_VERSION}")
project(OpenBeken)
idf_build_set_property(COMPILE_OPTIONS "-Wno-error=format-truncation" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-Wno-error=incompatible-pointer-types" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-Wno-error=pointer-sign" APPEND)
idf_build_set_property(COMPILE_OPTIONS "-Wno-error=main" APPEND)
