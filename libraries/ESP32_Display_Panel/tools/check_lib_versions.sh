#!/bin/bash

# Function: Check version format
# Input parameters: $1 The version number
# Return value: 0 if the version numbers are correct, 1 if the first version is incorrect,
check_version_format() {
    version_regex="^v[0-9]+\.[0-9]+\.[0-9]+$"

    if [[ ! $1 =~ $version_regex ]]; then
        return 1
    fi

    return 0
}

# Function: Check if a file exists
# Input parameters: $1 The file to check
# Return value: 0 if the file exists, 1 if the file does not exist
check_file_exists() {
    if [ ! -f "$1" ]; then
        echo "File '$1' not found."
        return 1
    fi
    return 0
}

# Function: Compare version numbers
# Input parameters: $1 The first version number, $2 The second version number
# Return value: 0 if the version numbers are equal, 1 if the first version is greater,
#               2 if the second version is greater,
compare_versions() {
    version_regex="^v[0-9]+\.[0-9]+\.[0-9]+$"

    version1=$(echo "$1" | cut -c 2-)  # Remove the 'v' at the beginning of the version number
    version2=$(echo "$2" | cut -c 2-)

    IFS='.' read -ra v1_parts <<< "$version1"
    IFS='.' read -ra v2_parts <<< "$version2"

    for ((i=0; i<${#v1_parts[@]}; i++)); do
        if [[ "${v1_parts[$i]}" -lt "${v2_parts[$i]}" ]]; then
            return 2
        elif [[ "${v1_parts[$i]}" -gt "${v2_parts[$i]}" ]]; then
            return 1
        fi
    done

    return 0
}

# Get the latest release version
latest_version="v0.0.0"
for i in "$@"; do
    case $i in
        --latest_version=*)
            latest_version="${i#*=}"
            shift
            ;;
        *)
            ;;
    esac
done
# Check the version format
check_version_format "${latest_version}"
result=$?
if [ ${result} -ne 0 ]; then
    echo "The latest release version (${latest_version}) format is incorrect."
    exit 1
fi
echo "Get the latest release version: ${latest_version}"

# Specify the directory path
target_directory="./"

echo "Checking directory: ${target_directory}"

echo "Checking file: library.properties"
# Check if "library.properties" file exists
check_file_exists "${target_directory}/library.properties"
if [ $? -ne 0 ]; then
    exit 1
fi
# Read the version information from the file
arduino_version=v$(grep -E '^version=' "${target_directory}/library.properties" | cut -d '=' -f 2)
echo "Get Arduino version: ${arduino_version}"
# Check the version format
check_version_format "${arduino_version}"
result=$?
if [ ${result} -ne 0 ]; then
    echo "Arduino version (${arduino_version}) format is incorrect."
    exit 1
fi

# Compare Arduino Library version with the latest release version
compare_versions "${arduino_version}" "${latest_version}"
result=$?
if [ ${result} -ne 1 ]; then
    if [ ${result} -eq 3 ]; then
        echo "Arduino version (${arduino_version}) is incorrect."
    else
        echo "Arduino version (${arduino_version}) is not greater than the latest release version (${latest_version})."
        exit 1
    fi
fi

echo "Checking file: idf_component.yml"
# Check if "idf_component.yml" file exists
check_file_exists "${target_directory}/idf_component.yml"
if [ $? -eq 0 ]; then
    # Read the version information from the file
    idf_version=v$(grep -E '^version:' "${target_directory}/idf_component.yml" | awk -F'"' '{print $2}')
    echo "Get IDF component version: ${idf_version}"
    # Check the version format
    check_version_format "${idf_version}"
    result=$?
    if [ ${result} -ne 0 ]; then
        echo "IDF component (${idf_version}) format is incorrect."
        exit 1
    fi
    # Compare IDF Component version with Arduino Library version
    compare_versions ${idf_version} ${arduino_version}
    result=$?
    if [ ${result} -ne 0 ]; then
        if [ ${result} -eq 3 ]; then
            echo "IDF component version (${idf_version}) is incorrect."
        else
        echo "IDF component version (${idf_version}) is not equal to the Arduino version (${arduino_version})."
            exit 1
        fi
    fi
    # Compare IDF Component version with the latest release version
    compare_versions ${idf_version} ${latest_version}
    result=$?
    if [ ${result} -ne 1 ]; then
        if [ ${result} -eq 3 ]; then
            echo "IDF component version (${idf_version}) is incorrect."
        else
            echo "IDF component version (${idf_version}) is not greater than the latest release version (${latest_version})."
            exit 1
        fi
    fi
fi
