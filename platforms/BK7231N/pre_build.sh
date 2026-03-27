# This script will be called just before starting build process for BK7231N
# It allows you to make changes to the SDK, for example..
# For example, you can use changed files in the SDK for the automated build during the checks for a PR without changing the SDK itself:
# So if your PR requires a changed define in SDK, you can do this here

#
#
# As an example you will find a script below which will copy all content of the "override"
# directory to the corresponding location in the SDK
#
DIRNAME=$(dirname $0);
echo "PREBUILD script! Executed from $DIRNAME!"

echo "Applying FINAL override (guaranteed)..."

CONFIG_FILE="src/obk_config.h"
OVERRIDE_FILE="src/_auto_override_config.h"

# tạo file override (luôn clean)
cat > $OVERRIDE_FILE << 'EOF'
#pragma once

// ===== FINAL FORCE OVERRIDE =====
#undef ENABLE_OBK_BERRY
//#undef ENABLE_LITTLEFS ⚠️ nên giữ
//#undef ENABLE_OBK_SCRIPTING ⚠️ nên giữ
#undef ENABLE_NTP 	

#undef ENABLE_DRIVER_DHT
#undef ENABLE_DRIVER_AHT2X
#undef ENABLE_DRIVER_DS1820
#undef ENABLE_DRIVER_CHT83XX
#undef ENABLE_DRIVER_KP18058
#undef ENABLE_DRIVER_ADCSMOOTHER

#undef ENABLE_DRIVER_SSDP
#undef ENABLE_DRIVER_WEMO
#undef ENABLE_DRIVER_HUE
#undef ENABLE_DRIVER_DDP

EOF

# remove include cũ nếu có (tránh duplicate)
sed -i '/_auto_override_config.h/d' "$CONFIG_FILE"

# 👉 QUAN TRỌNG NHẤT: include ở CUỐI FILE
echo '#include "_auto_override_config.h"' >> "$CONFIG_FILE"

echo "Override injected at END of obk_config.h ✔"






# ===== phần override gốc giữ nguyên =====

# allow whitespace in file or path, so take only newline as seperator
OFS=$IFS
IFS='
'
for X in $(find platforms/BK7231N/override/ -type f);do
#	# script is executed from main app directory, so take found file and path as source
	S=${X};
#	# destination is path stripped from path to override
#	# so inside "override" we have the full path to the file
#	# starting with "sdk/OpenBK7231N/..."
	D=${X#platforms/BK7231N/override/};
#	# if file is present, we replace it, otherwise file is added ...
	[ -e $D ] && echo "PREBUILD: replacing file\n\t$D\n\twith file\n\t$S" || echo "PREBUILD: adding file\n\t$S\n\tas\n\t$D"
	cp $S $D;
done
## restore IFS to whatever it was before ...
IFS=$OFS

# you can also use all other commands to change files, like
# sed -i "s/#define FOO bar/#define FOO baz/" sdk/OpenBK7231N/platforms/bk7231n/bk7231n_os/beken378/file_to_change.c
# or, let's assume you made a local change to your SDK
# and make a diff from that change (inside sdk/OpenBK7231N/)
# git diff > ../../platforms/BK7231N/my_change.diff
# ( or make the diff and copy this file to platforms/BK7231N)
#
# and then in pre_build.sh you apply this patch with:
#
# patch -p 1 -d sdk/OpenBK7231N < platforms/BK7231N/my_change.diff
