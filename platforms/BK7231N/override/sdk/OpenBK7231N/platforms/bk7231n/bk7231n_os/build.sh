#!/bin/sh
APP_BIN_NAME=$1
APP_VERSION=$2
TARGET_PLATFORM=$3
USER_CMD=$4
BUILD_MODE=$5

echo APP_BIN_NAME=$APP_BIN_NAME
echo APP_VERSION=$APP_VERSION
echo TARGET_PLATFORM=$TARGET_PLATFORM
echo USER_CMD=$USER_CMD
echo BUILD_MODE=$BUILD_MODE

USER_SW_VER=`echo $APP_VERSION | cut -d'-' -f1`

echo "Start Compile"
set -e

SYSTEM=`uname -s`
echo "system:"$SYSTEM
if [ $SYSTEM = "Linux" ]; then
	TOOL_DIR=package_tool/linux
	OTAFIX=${TOOL_DIR}/otafix
	ENCRYPT=${TOOL_DIR}/encrypt
	BEKEN_PACK=${TOOL_DIR}/beken_packager
	RT_OTA_PACK_TOOL=${TOOL_DIR}/rt_ota_packaging_tool_cli
	TY_PACKAGE=${TOOL_DIR}/package
	ENCRYPT_NEW=${TOOL_DIR}/cmake_encrypt_crc
else
	TOOL_DIR=package_tool/windows
	OTAFIX=${TOOL_DIR}/otafix.exe
	ENCRYPT=${TOOL_DIR}/encrypt.exe
	BEKEN_PACK=${TOOL_DIR}/beken_packager.exe
	RT_OTA_PACK_TOOL=${TOOL_DIR}/rt_ota_packaging_tool_cli.exe
	TY_PACKAGE=${TOOL_DIR}/package.exe
	ENCRYPT_NEW=${TOOL_DIR}/cmake_encrypt_crc.exe
fi

APP_PATH=../../../apps

for i in `find ${APP_PATH}/$APP_BIN_NAME/src -type d`
do
    rm -rf $i/*.o
done

# for i in `find ../tuya_common -type d`
# do
#     rm -rf $i/*.o
# done

# for i in `find ../../../components -type d`
# do
#     rm -rf $i/*.o
# done

if [ -z $CI_PACKAGE_PATH ]; then
    echo "not is ci build"
else
	make APP_BIN_NAME=$APP_BIN_NAME USER_SW_VER=$USER_SW_VER APP_VERSION=$APP_VERSION clean -C ./
fi

make APP_BIN_NAME=$APP_BIN_NAME USER_SW_VER=$USER_SW_VER APP_VERSION=$APP_VERSION $USER_CMD -j -C ./

echo "Start Combined"
cp ${APP_PATH}/$APP_BIN_NAME/output/$APP_VERSION/${APP_BIN_NAME}_${APP_VERSION}.bin tools/generate/

cd tools/generate/

# cp [sourceFile] [destinationFile]
cp ${APP_BIN_NAME}_${APP_VERSION}.bin ${APP_BIN_NAME}_${APP_VERSION}_zeroKeys.bin

if [ "$BUILD_MODE" = "zerokeys" ]; then
	echo "Using zero keys mode - for those non-Tuya devices"
	./${ENCRYPT_NEW} -crc ${APP_BIN_NAME}_${APP_VERSION}.bin 4862379A 8612784B 85C5E258 75754528 10000
	#python mpytools.py bk7231n_bootloader_zero_keys.bin ${APP_BIN_NAME}_${APP_VERSION}_enc.bin
	python mpytools.py bk7231n_bootloader_enc.bin ${APP_BIN_NAME}_${APP_VERSION}_enc.bin
else
	echo "Using usual Tuya path"
	./${ENCRYPT_NEW} -crc ${APP_BIN_NAME}_${APP_VERSION}.bin 4862379A 8612784B 85C5E258 75754528 10000
	python mpytools.py bk7231n_bootloader_enc.bin ${APP_BIN_NAME}_${APP_VERSION}_enc.bin
fi


./${BEKEN_PACK} config.json

echo "End Combined"
cp all_1.00.bin ${APP_BIN_NAME}_QIO_${APP_VERSION}.bin
rm all_1.00.bin

cp ${APP_BIN_NAME}_${APP_VERSION}_enc_uart_1.00.bin ${APP_BIN_NAME}_UA_${APP_VERSION}.bin
rm ${APP_BIN_NAME}_${APP_VERSION}_enc_uart_1.00.bin

#generate ota file
echo "generate ota file"
./${RT_OTA_PACK_TOOL} -f ${APP_BIN_NAME}_${APP_VERSION}.bin -v $CURRENT_TIME -o ${APP_BIN_NAME}_${APP_VERSION}.rbl -p app -c gzip -s aes -k 0123456789ABCDEF0123456789ABCDEF -i 0123456789ABCDEF
./${TY_PACKAGE} ${APP_BIN_NAME}_${APP_VERSION}.rbl ${APP_BIN_NAME}_UG_${APP_VERSION}.bin $APP_VERSION 
echo rm ${APP_BIN_NAME}_${APP_VERSION}.rbl
rm ${APP_BIN_NAME}_${APP_VERSION}.bin
rm ${APP_BIN_NAME}_${APP_VERSION}.cpr
rm ${APP_BIN_NAME}_${APP_VERSION}.out
rm ${APP_BIN_NAME}_${APP_VERSION}_enc.bin

echo "ug_file size:"
ls -l ${APP_BIN_NAME}_UG_${APP_VERSION}.bin | awk '{print $5}'
if [ `ls -l ${APP_BIN_NAME}_UG_${APP_VERSION}.bin | awk '{print $5}'` -gt 679936 ];then
	echo "**********************${APP_BIN_NAME}_$APP_VERSION.bin***************"
	echo "************************** too large ********************************"
	rm ${APP_BIN_NAME}_UG_${APP_VERSION}.bin
	rm ${APP_BIN_NAME}_UA_${APP_VERSION}.bin
	rm ${APP_BIN_NAME}_QIO_${APP_VERSION}.bin
	exit 1
fi

if [ "$BUILD_MODE" = "zerokeys" ]; then
	TEMP_FILE=$(mktemp)
	
	dd if="bk7231n_bootloader_zero_keys.bin" bs=1 count=65536 of="$TEMP_FILE"
	dd if="${APP_BIN_NAME}_QIO_${APP_VERSION}.bin" bs=1 skip=65536 of="$TEMP_FILE" seek=65536
	mv "$TEMP_FILE" "${APP_BIN_NAME}_QIO_${APP_VERSION}.bin"
	echo "Successfully overwrote bootloader."
fi

echo "$(pwd)"
cp ${APP_BIN_NAME}_${APP_VERSION}.rbl ../../${APP_PATH}/$APP_BIN_NAME/output/$APP_VERSION/${APP_BIN_NAME}_${APP_VERSION}.rbl
cp ${APP_BIN_NAME}_UG_${APP_VERSION}.bin ../../${APP_PATH}/$APP_BIN_NAME/output/$APP_VERSION/${APP_BIN_NAME}_UG_${APP_VERSION}.bin
cp ${APP_BIN_NAME}_UA_${APP_VERSION}.bin ../../${APP_PATH}/$APP_BIN_NAME/output/$APP_VERSION/${APP_BIN_NAME}_UA_${APP_VERSION}.bin
cp ${APP_BIN_NAME}_QIO_${APP_VERSION}.bin ../../${APP_PATH}/$APP_BIN_NAME/output/$APP_VERSION/${APP_BIN_NAME}_QIO_${APP_VERSION}.bin



# 
# 	BK7231M steps (zero keys)
# 
echo "Will do extra step - for zero keys/dogness"
# Blank-start with zero keys bin
# cp [sourceFile] [destinationFile]
cp ${APP_BIN_NAME}_${APP_VERSION}_zeroKeys.bin ${APP_BIN_NAME}_${APP_VERSION}.bin
# Apply keys
echo "Will do zero keys encrypt"
# This will generate ${APP_BIN_NAME}_${APP_VERSION}_enc.bin
./${ENCRYPT_NEW} -crc ${APP_BIN_NAME}_${APP_VERSION}.bin 4862379A 8612784B 85C5E258 75754528 10000
echo "Will do zero mpytools.py to generate config.json"
# python mpytools.py [BootloaderFile] [AppFile]
python mpytools.py bk7231n_bootloader_enc.bin ${APP_BIN_NAME}_${APP_VERSION}_enc.bin
echo "Will do zero BEKEN_PACK"
./${BEKEN_PACK} config.json
echo "Will do zero qio"
cp all_1.00.bin ${APP_BIN_NAME}_QIO_${APP_VERSION}.bin
cp ${APP_BIN_NAME}_QIO_${APP_VERSION}.bin ../../${APP_PATH}/$APP_BIN_NAME/output/$APP_VERSION/OpenBK7231M_QIO_${APP_VERSION}.bin
cp ${APP_BIN_NAME}_UA_${APP_VERSION}.bin ../../${APP_PATH}/$APP_BIN_NAME/output/$APP_VERSION/OpenBK7231M_UA_${APP_VERSION}.bin


# 
# 	UASCENT steps (4862379A 8612784B 85C5E258 75754528)
# 
# Copy blank bootloader just to have it  with "uascent" in name
# cp [sourceFile] [destinationFile]
cp bk7231n_bootloader.bin bk7231n_bootloader_uascent.bin
# Call encrypt like Divadiow did
# ENCRYPT_NEW is cmake_encrypt_crc.exe
# this shall generate bk7231n_bootloader_uascent_enc.bin for bk7231n_bootloader_uascent.bin
./${ENCRYPT_NEW} -crc bk7231n_bootloader_uascent.bin 4862379A 8612784B 85C5E258 75754528
# Copy blank-start App with zero keys bin
# cp [sourceFile] [destinationFile]
cp ${APP_BIN_NAME}_${APP_VERSION}_zeroKeys.bin ${APP_BIN_NAME}_${APP_VERSION}.bin
# Apply keys to the App
echo "Will do UASCENT encrypt"
# This will generate ${APP_BIN_NAME}_${APP_VERSION}_enc.bin
./${ENCRYPT_NEW} -crc ${APP_BIN_NAME}_${APP_VERSION}.bin 4862379A 8612784B 85C5E258 75754528 10000
# Use mpytools.py to generate config.json for encrypted bootloader and encrypted app	
echo "Will do UASCENT mpytools.py to generate config.json"
# python mpytools.py [BootloaderFile] [AppFile]
python mpytools.py bk7231n_bootloader_uascent_enc.bin ${APP_BIN_NAME}_${APP_VERSION}_enc.bin
# Use Beken Pack on created config.json to combine bootloader and app together into all_1.00.bin
echo "Will do UASCENT BEKEN_PACK"
./${BEKEN_PACK} config.json
echo "Will do UASCENT qio"
cp all_1.00.bin ${APP_BIN_NAME}_QIO_${APP_VERSION}.bin
cp ${APP_BIN_NAME}_QIO_${APP_VERSION}.bin ../../${APP_PATH}/$APP_BIN_NAME/output/$APP_VERSION/OpenBK7231N_UASCENT_QIO_${APP_VERSION}.bin
cp ${APP_BIN_NAME}_UA_${APP_VERSION}.bin ../../${APP_PATH}/$APP_BIN_NAME/output/$APP_VERSION/OpenBK7231N_UASCENT_UA_${APP_VERSION}.bin
	
	

echo "*************************************************************************"
echo "*************************************************************************"
echo "*************************************************************************"
echo "*********************${APP_BIN_NAME}_$APP_VERSION.bin********************"
echo "*************************************************************************"
echo "**********************COMPILE SUCCESS************************************"
echo "*************************************************************************"

FW_NAME=$APP_NAME
if [ -n $CI_IDENTIFIER ]; then
        FW_NAME=$CI_IDENTIFIER
fi

if [ -z $CI_PACKAGE_PATH ]; then
    echo "not is ci build"
	exit
else
	mkdir -p ${CI_PACKAGE_PATH}

   cp ../../${APP_PATH}/$APP_BIN_NAME/output/$APP_VERSION/${APP_BIN_NAME}_UG_${APP_VERSION}.bin ${CI_PACKAGE_PATH}/$FW_NAME"_UG_"$APP_VERSION.bin
   cp ../../${APP_PATH}/$APP_BIN_NAME/output/$APP_VERSION/${APP_BIN_NAME}_UA_${APP_VERSION}.bin ${CI_PACKAGE_PATH}/$FW_NAME"_UA_"$APP_VERSION.bin
   cp ../../${APP_PATH}/$APP_BIN_NAME/output/$APP_VERSION/${APP_BIN_NAME}_QIO_${APP_VERSION}.bin ${CI_PACKAGE_PATH}/$FW_NAME"_QIO_"$APP_VERSION.bin
   cp ../../${APP_PATH}/$APP_BIN_NAME/output/$APP_VERSION/${APP_BIN_NAME}_${APP_VERSION}.asm ${CI_PACKAGE_PATH}/$FW_NAME"_"$APP_VERSION.asm
   cp ../../${APP_PATH}/$APP_BIN_NAME/output/$APP_VERSION/${APP_BIN_NAME}_${APP_VERSION}.axf ${CI_PACKAGE_PATH}/$FW_NAME"_"$APP_VERSION.axf
   cp ../../${APP_PATH}/$APP_BIN_NAME/output/$APP_VERSION/${APP_BIN_NAME}_${APP_VERSION}.map ${CI_PACKAGE_PATH}/$FW_NAME"_"$APP_VERSION.map
fi