# This script will be called just before starting build process for LN882H
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
# allow whitspace in file or path, so take only newline as seperator
#OFS=$IFS
#IFS='
#'
#for X in $(find platforms/LN882H/override/ -type f);do
#	# script is executed from main app directory, so take found file and path as source
#	S=${X};
#	# destination is path stripped from path to override
#	# so inside "override" we have the full path to the file
#	# starting with "sdk/OpenLN882H/..."
#	D=${X#platforms/LN882H/override/};
#	# if file is present, we replace it, otherwise file is added ...
#	[ -e $D ] && echo "PREBUILD: replacing file\n\t$D\n\twith file\n\t$S" || echo "PREBUILD: adding file\n\t$S\n\tas\n\t$D"
#	cp $S $D;
#done
## restore IFS to whatever it was before ...
#IFS=$OFS

# you can also use all other commands to change files, like
# sed -i "s/#define FOO bar/#define FOO baz/" sdk/OpenLN882H/project/OpenBeken/cfg/file_to_change.h
# or, let's assume you made a local change to your SDK
# and make a diff from that change (inside sdk/OpenLN882H/)
# git diff > ../../platforms/LN882H/my_change.diff
# ( or make the diff and copy this file to platforms/LN882H)
#
# and then in pre_build.sh you apply this patch with:
#
# patch -p 1 -d sdk/OpenLN882H < platforms/LN882H/my_change.diff

