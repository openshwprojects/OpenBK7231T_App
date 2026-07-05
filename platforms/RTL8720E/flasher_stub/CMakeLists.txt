set_property(GLOBAL PROPERTY ALLOW_DUPLICATE_CUSTOM_TARGETS TRUE)
ameba_add_image(target_floader p_EXCLUDE_FROM_ALL)
# variable c_CURRENT_IMAGE value is target_loader_${PROJECT_NAME}

###############################  ADD COMPONENT  ###################################

ameba_add_subdirectory(${c_CMPT_CRASHDUMP_DIR})
ameba_add_subdirectory(${c_CMPT_CHIPINFO_DIR})
ameba_add_subdirectory(${c_CMPT_SOC_DIR}/lib/bootloader lib_bootloader)
ameba_add_subdirectory(${c_CMPT_FILE_SYSTEM_DIR})
ameba_add_subdirectory(${c_CMPT_OS_DIR}/freertos/os_wrapper)

###############################  GENERATE IMAGE  ###################################
#---------------- PRE LINK -------------------#

add_custom_command(
  TARGET ${c_CURRENT_IMAGE} PRE_LINK
  COMMAND ${CMAKE_C_COMPILER} -E -P -xc -c -DGENERATE_FLOADER_LD ${CMAKE_CURRENT_SOURCE_DIR}/../../RTL8721DA/flasher_stub/stub.ld -I ${c_SOC_PROJECT_DIR} -o ${CMAKE_CURRENT_BINARY_DIR}/rlx8721d.ld -I ${c_MENUCONFIG_DIR}
  COMMAND ${CMAKE_COMMAND} -E cat ${c_LINK_ROM_SYMBOL_S} >> ${CMAKE_CURRENT_BINARY_DIR}/rlx8721d.ld
  COMMENT "build flashloader linkerscript rlx8721d.ld"
)

#----------------- LINKING -------------------#

target_link_libraries(${c_CURRENT_IMAGE}
	PRIVATE
	-T${CMAKE_CURRENT_BINARY_DIR}/rlx8721d.ld
	${c_MCU_PROJ_CONFIG}
	-Wl,--gc-sections
	-Wl,-Map=${CMAKE_CURRENT_BINARY_DIR}/text.map
	-Wl,--whole-archive
	$<TARGET_PROPERTY:${c_MCU_PROJ_CONFIG},${c_CURRENT_IMAGE}_libraries>
	$<TARGET_PROPERTY:${c_MCU_PROJ_CONFIG},${c_CURRENT_IMAGE}_whole_archive_libs>
	-Wl,--no-whole-archive

	$<TARGET_PROPERTY:${c_MCU_PROJ_CONFIG},${c_CURRENT_IMAGE}_no_whole_archive_libs>

	${c_LINK_ROM_LIB}
)
#--------------- POST BUILD ------------------#

add_custom_command(TARGET ${c_CURRENT_IMAGE} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E make_directory ${c_SDK_IMAGE_TARGET_DIR}
	COMMAND ${CMAKE_COMMAND} -E touch ${c_SDK_IMAGE_TARGET_DIR}/.gitignore
	COMMAND ${CMAKE_COMMAND} -E copy text.map ${c_SDK_IMAGE_TARGET_DIR}/text_loader.map
	COMMAND ${CMAKE_COMMAND} -E copy ${c_CURRENT_IMAGE}.axf ${c_SDK_IMAGE_TARGET_DIR}/target_loader.axf
	COMMAND ${CMAKE_NM} ${c_SDK_IMAGE_TARGET_DIR}/target_loader.axf | sort > ${c_SDK_IMAGE_TARGET_DIR}/target_loader.map
	COMMAND ${CMAKE_OBJDUMP} -d ${c_SDK_IMAGE_TARGET_DIR}/target_loader.axf > ${c_SDK_IMAGE_TARGET_DIR}/target_loader.asm
	COMMAND ${CMAKE_COMMAND} -E copy ${c_SDK_IMAGE_TARGET_DIR}/target_loader.axf ${c_SDK_IMAGE_TARGET_DIR}/target_pure_loader.axf
	COMMAND ${CMAKE_STRIP} ${c_SDK_IMAGE_TARGET_DIR}/target_pure_loader.axf

	COMMAND ${CMAKE_COMMAND} -E echo "  BIN      stub.bin"

	COMMAND ${CMAKE_COMMAND}
		# common variables
		-Dc_BASEDIR=${c_BASEDIR}  # dir of sdk's root
		-Dc_CMAKE_FILES_DIR=${c_CMAKE_FILES_DIR} # dir of cmake scripts
		-Dc_SOC_PROJECT_DIR=${c_SOC_PROJECT_DIR} # dir of soc project
		-Dc_MCU_PROJECT_DIR=${c_MCU_PROJECT_DIR} # dir of current mcu project
		-Dc_MCU_PROJECT_NAME=${c_MCU_PROJECT_NAME} # name of current mcu project
		-Dc_MCU_KCONFIG_FILE=${c_MCU_KCONFIG_FILE} # file of current project's kconfig file
		-Dc_SDK_IMAGE_TARGET_DIR=${c_SDK_IMAGE_TARGET_DIR} # dir of image output

		# user's variables
		-DFINAL_IMAGE_DIR=${FINAL_IMAGE_DIR}
		-DBUILD_TYPE=${BUILD_TYPE}
		-DANALYZE_MP_IMG=${ANALYZE_MP_IMG}
		-DDAILY_BUILD=${DAILY_BUILD}
		-DEXTERN_DIR=${EXTERN_DIR}

		# special variables for current image
		-DIMGTOOL_LOADER=y

		# cmake toolchain variables
		-DIMAGESCRIPTDIR=${IMAGESCRIPTDIR}
		-DCMAKE_SIZE=${CMAKE_SIZE}
		-DCMAKE_OBJCOPY=${CMAKE_OBJCOPY}

		-P ${c_SDK_IMAGE_TARGET_DIR}/../make/image1/${c_POST_BUILD_SCRIPT}

	COMMAND ${CMAKE_COMMAND} -E copy ${c_SDK_IMAGE_TARGET_DIR}/ram_1.bin           ${CMAKE_CURRENT_SOURCE_DIR}/stub.bin
	COMMAND gzip ${CMAKE_CURRENT_SOURCE_DIR}/stub.bin
	COMMAND mv ${CMAKE_CURRENT_SOURCE_DIR}/stub.bin.gz ${CMAKE_CURRENT_SOURCE_DIR}/RTL8720E_Stub.bin
)


################################  EXTRA PROCESS  ###################################
set(clean_files
)

set(link_deps
	${c_LAYOUT_LD}
	${c_IMG1_LD}
	${c_LINK_ROM_SYMBOL_S}
)

set_target_properties(${c_CURRENT_IMAGE} PROPERTIES
	#WARNING: values for these parameter MUST be placed inside a pair of quotes
	# ADDITIONAL_CLEAN_FILES
	#     "${clean_files}"
	LINK_DEPENDS
		"${link_deps}"
)
################################  CUSTOM TARGET  ###################################

add_custom_target(
	flashloader
	COMMENT "build flashloader only"
	DEPENDS ${c_CURRENT_IMAGE}
)
set(public_includes)                #public include directories, NOTE: relative path is OK
set(public_definitions)             #public definitions
set(public_libraries)               #public libraries(files), NOTE: linked with whole-archive options

#----------------------------------------#
# Component public part, user config begin

# You may use if-else condition to set or update predefined variable above

# Component public part, user config end
#----------------------------------------#

#WARNING: Fixed section, DO NOT change!
ameba_global_include(${public_includes})
ameba_global_define(${public_definitions})
ameba_global_library(${public_libraries}) #default: whole-archived

##########################################################################################
## * This part defines private part of the component
## * Private part is used to build target of current component
## * NOTE: The build API guarantees the global build configures(mentioned above)
## *       applied to the target automatically. So if any configure was already added
## *       to public above, it's unnecessary to add again below.

#NOTE: User defined section, add your private build configures here
# You may use if-else condition to set these predefined variable
# They are only for ameba_add_internal_library/ameba_add_external_app_library/ameba_add_external_soc_library
set(private_sources)                 #private source files, NOTE: relative path is OK
set(private_includes)                #private include directories, NOTE: relative path is OK
set(private_definitions)             #private definitions
set(private_compile_options)         #private compile_options

#------------------------------#
# Component private part, user config begin

ameba_list_append(private_sources
	../../RTL8721DA/flasher_stub/flasher_stub.c
	${c_CMPT_SWLIB_DIR}/log.c
	${c_CMPT_FWLIB_DIR}/ram_km4/ameba_arch.c
	${c_CMPT_FWLIB_DIR}/ram_common/ameba_otp.c
#	${c_CMPT_FWLIB_DIR}/ram_common/ameba_ipc_ram.c
	${c_BASEDIR}/component/soc/usrcfg/amebalite/ameba_bootcfg.c
	${c_BASEDIR}/component/soc/usrcfg/amebalite/ameba_flashcfg.c
	${c_CMPT_SOC_DIR}/hal/src/flash_api.c
	${c_CMPT_SOC_DIR}/misc/ameba_pmu.c
)

ameba_list_append(private_sources
	${c_FREERTOS_DIR}/tasks.c
	${c_FREERTOS_DIR}/list.c
	${c_FREERTOS_DIR}/croutine.c
	${c_FREERTOS_DIR}/queue.c
	${c_FREERTOS_DIR}/timers.c
	${c_FREERTOS_DIR}/event_groups.c
	${c_FREERTOS_DIR}/stream_buffer.c
	${c_FREERTOS_DIR}/portable/MemMang/heap_5.c
)

set(portable_soc_mcu_dir ${c_FREERTOS_DIR}/portable/GCC/${c_SOC_TYPE_CAMEL}_${c_MCU_TYPE_UPPER})
ameba_list_append(private_sources
	${portable_soc_mcu_dir}/non_secure/port.c
	${portable_soc_mcu_dir}/non_secure/portasm.c
)
# Component private part, user config end
#------------------------------#

#WARNING: Select right API based on your component's release/not-release/standalone

###NOTE: For open-source component, always build from source
ameba_add_internal_library(flashloader
	p_SOURCES
		${private_sources}
	p_INCLUDES
		${private_includes}
	p_DEFINITIONS
		${private_definitions}
	p_COMPILE_OPTIONS
		${private_compile_options}
	p_DEPENDENCIES
		${c_BUILD_INFO}
)

target_compile_options(
	${c_MCU_PROJ_CONFIG} INTERFACE
	-Os
	-DLFS_NO_DEBUG -DLFS_NO_WARN -DLFS_NO_ERROR -DLFS_NO_ASSERT
)

##########################################################################################