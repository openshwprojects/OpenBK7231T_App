# This script will be called just before starting build process for BK7231N
# It allows you to make changes to the SDK, for example..
# For example, you can use changed files in the SDK for the automated build during the checks for a PR without changing the SDK itself:
# So if your PR requires a changed define in SDK, you can do this here

#
#
# As an example you will find a script below which will copy all content of the "override"
# directory to the corresponding location in the SDK
#
#DIRNAME=$(dirname $0);
#echo "PREBUILD script! Executed from $DIRNAME!"
# allow whitspace in file or path, so take only newline as seperator
#OFS=$IFS
#IFS='
#'
#for X in $(find platforms/BK7231N/override/ -type f);do
#	# script is executed from main app directory, so take found file and path as source
#	S=${X};
#	# destination is path stripped from path to override
#	# so inside "override" we have the full path to the file
#	# starting with "sdk/OpenBK7231N/..."
#	D=${X#platforms/BK7231N/override/};
#	# if file is present, we replace it, otherwise file is added ...
#	[ -e $D ] && echo "PREBUILD: replacing file\n\t$D\n\twith file\n\t$S" || echo "PREBUILD: adding file\n\t$S\n\tas\n\t$D"
#	cp $S $D;
#done
## restore IFS to whatever it was before ...
#IFS=$OFS

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

