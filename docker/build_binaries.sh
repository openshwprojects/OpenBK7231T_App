#!/usr/bin/env bash

# NOTE - This script requires cash 4 or later

OPENBK_APP_REPO=${OPENBK_APP_REPO:-"https://github.com/openshwprojects/OpenBK7231T_App.git"}

declare -A sdk_repositories
sdk_repositories=(
#        ["OpenBK7231T"]="https://github.com/openshwprojects/OpenBK7231T.git"
        ["OpenBK7231N"]="https://github.com/openshwprojects/OpenBK7231N.git"
#        ["XR809"]="https://github.com/openshwprojects/OpenXR809.git"
    )

declare -A app_folder
app_folder=(
        ["OpenBK7231T"]="apps"
        ["OpenBK7231N"]="apps"
        ["XR809"]="project/oxr_sharedApp"
    )

declare -A app_link_name
app_link_name=(
        ["OpenBK7231T"]="OpenBK7231T_App"
        ["OpenBK7231N"]="OpenBK7231N_App"
        ["XR809"]="shared"
    )


declare -A build_script
build_script=(
        ["OpenBK7231T"]="b.sh"
        ["OpenBK7231N"]="b.sh"
        ["XR809"]="build_oxr_sharedApp.sh"
    )





# if [ ! -d ${BUILD_DIRPATH}/output/ ]; then
#     mkdir -p ${BUILD_DIRPATH}/output/
# fi

for sdk in "${!sdk_repositories[@]}"
do
    cd /build
    if [ ! -d ${sdk} ]
    then
        echo "cloning sdk ${sdk} repository = ${sdk_repositories[$sdk]}"
        git clone ${sdk_repositories[$sdk]} ${sdk}
    else
        echo "Pulling update to ${sdk}"
        cd ${sdk}
        git pull
        cd ..
    fi

    app_link_relative_path=${sdk}/${app_folder[$sdk]}/${app_link_name[$sdk]}

    if [ ! -d ${app_link_relative_path} ]
    then
        echo "creating link to OpenBK7231T_App at: ${app_link_relative_path}"
        ln -s /${OPENBK7231T_APP_NAME} ${app_link_relative_path}
    fi
    echo "Building ${sdk} with script: ${build_script[$sdk]}"
    cd ${sdk}
    # make sure build script is executable
    chmod +x ${build_script[$sdk]}
    # run the build script
    ./${build_script[$sdk]}
done

echo "I am: $(whoami)"