# This script will be called just before starting build process for OpenXR872
# It allows you to make changes to the SDK, for example..
# For example, you can use changed files in the SDK for the automated build during the checks for a PR without changing the SDK itself:
# So your PR needs a modified define in the SDK, for example ? This script can make this change directly before the build.
#
#
# As an example you will find a script below which will copy all content of the "override"
# directory to the corresponding location in the SDK
#
#DIRNAME=$(dirname $0);
#echo "PREBUILD script! Executed from $DIRNAME!"
# allow whitespace in file or path, so take only newline as seperator
#OFS=$IFS
#IFS='
#'
#for X in $(find platforms/OpenXR872/override/ -type f);do
#	# script is executed from main app directory, so take found file and path as source
#	S=${X};
#	# destination is path stripped from path to override
#	# so inside "override" we have the full path to the file
#	# starting with "sdk/OpenOpenXR872/..."
#	D=${X#platforms/OpenXR872/override/};
#	# if file is present, we replace it, otherwise file is added ...
#	[ -e $D ] && echo "PREBUILD: replacing file\n\t$D\n\twith file\n\t$S" || echo "PREBUILD: adding file\n\t$S\n\tas\n\t$D"
#	cp $S $D;
#done
## restore IFS to whatever it was before ...
#IFS=$OFS
# you can also use all other commands to change files, like
# sed -i "s/#define FOO bar/#define FOO baz/" sdk/OpenOpenXR872/project/OpenBeken/cfg/file_to_change.h
# or, let's assume you made a local change to your SDK
# and make a diff from that change (inside sdk/OpenOpenXR872/)
# git diff > ../../platforms/OpenXR872/my_change.diff
# ( or make the diff and copy this file to platforms/OpenXR872)
#
# and then in pre_build.sh you apply this patch with:
#
# patch -p 1 -d sdk/OpenOpenXR872 < platforms/OpenXR872/my_change.diff

# don't include SDKs driver to avoid confict for SSD1306_Init
#/home/runner/gcc-arm-none-eabi-8.2019.3-linux-x64/gcc-arm-none-eabi-8-2019-q3-update/bin/../lib/gcc/arm-none-eabi/8.3.1/../../../../arm-none-eabi/bin/ld: ../../../../lib/libcomponent.a(ssd1306.o): in function `SSD1306_Init':
#ssd1306.c:(.text.SSD1306_Init+0x0): multiple definition of `SSD1306_Init'; ../shared/src/driver/drv_ssd1306.o:drv_ssd1306.c:(.text.SSD1306_Init+0x0): first defined here
#collect2: error: ld returned 1 exit status

sed -i "s%^[ \t]*SUBDIRS += driver/component%//&%" sdk/OpenXR872/src/Makefile
sed -i "s%-lcomponent%%" sdk/OpenXR872/project/project.mk

