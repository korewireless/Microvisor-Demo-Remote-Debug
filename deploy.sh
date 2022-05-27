#!/usr/bin/env bash

#
# deploy
#
# Upload and deploy Microvisor application code
#
# @author    Tony Smith
# @copyright 2022, Twilio
# @version   1.5.2
# @license   MIT
#

# GLOBALS
app_dir="demo"
private_key_path="build/${app_dir}/debug_auth_priv_key.pem"
zip_path="build/${app_dir}/mv-remote-debug-demo.zip"
cmake_path="${app_dir}/CMakeLists.txt"
do_log=0
do_build=1
do_deploy=1
do_update=1
public_key_path=NONE

# NOTE
# This script the build directory is called 'build' and exists within
# the project directory. If it is used to build the app, this will be
# the case. You can pass in alternative target for the build product
# eg. './deploy.sh build_alt/app/my_app.zip'

# FUNCTIONS
show_help() {
    echo -e "Usage:\n"
    echo -e "  deploy [-l] [-h] /optional/path/to/Microvisor/app/bunde.zip\n"
    echo -e "Options:\n"
    echo "  --log / -l           After deployment, start log streaming. Default: no logging"
    echo "  --public-key {path}  /path/to/remote/debugging/public/key.pem"
    echo "  --private-key {path} /path/to/remote/debugging/private/key.pem"
    echo "  -k                   Start log streaming immediately; do not build or deploy"
    echo "  -d                   Deploy without a build"
    echo "  -h / --help          Show this help screen"
    echo
}

stream_log() {
    echo -e "\nLogging from ${MV_DEVICE_SID}..."
    twilio microvisor:logs:stream "${MV_DEVICE_SID}"
}

build_app() {
    if [[ "${public_key_path}" != "NONE" ]]; then
        cmake -S . -B build -D "RD_PUBLIC_KEYPATH:STRING=${public_key_path}"
    else
        cmake -S . -B build
    fi

    cmake --build build --clean-first > /dev/null

    if [[ $? -eq 0 ]]; then
        echo "App built"
    else
        echo "[ERROR] Could not build the app... exiting"
        exit 1
    fi
}

update_build_number() {
    build_val=$(grep 'set(BUILD_NUMBER "' ${cmake_path})
    old_num=$(echo "${build_val}" | cut -d '"' -s -f 2)
    ((new_num=old_num+1))

    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        sed -i "s|BUILD_NUMBER \"${old_num}\"|BUILD_NUMBER \"${new_num}\"|" "${cmake_path}"
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        # macOS requires slightly different syntax from Unix
        sed -i '' "s|BUILD_NUMBER \"${old_num}\"|BUILD_NUMBER \"${new_num}\"|" "${cmake_path}"
    else
        echo "[ERROR] Unknown OS... build number not incremented"
    fi
}

# RUNTIME START
arg_is_value=0
for arg in "$@"; do
    check_arg=${arg,,}
    if [[ ${arg_is_value} -gt 0 ]]; then
        case "${arg_is_value}" in
            1) private_key_path="${arg}" ; echo "Remote Debugging private key: ${private_key_path}" ;;
            2) public_key_path="${arg}"  ; echo "Remote Debugging public key: ${public_key_path}"   ;;
            *) echo "[Error] Unknown argument" exit 1 ;;
        esac
        arg_is_value=0
        continue
    fi

    if [[ "${check_arg}" = "--log" || "${check_arg}" = "-l" ]]; then
        do_log=1
    elif [[ "${check_arg}" = "--private-key" ]]; then
        arg_is_value=1
        continue
    elif [[ "${check_arg}" = "--public-key" ]]; then
        arg_is_value=2
        continue
    elif [[ "${check_arg}" = "-k" ]]; then
        do_log=1
        do_deploy=0
        do_build=0
    elif [[ "${check_arg}" = "-d" ]]; then
        do_build=0
    elif [[ "${check_arg}" = "--help" || "${check_arg}" = "-h" ]]; then
        show_help
        exit 0
    else
        zip_path="${arg}"
    fi
done

if [[ ${do_build} -eq 1 ]]; then
    [[ ${do_update} -eq 1 ]] && update_build_number
    build_app
fi

if [[ ${do_deploy} -eq 1 ]]; then
    # Check we have what looks like a bundle
    extension="${zip_path##*.}"
    if [[ "${extension}" != "zip" ]]; then
        echo "[ERROR] ${zip_path} does not indicate a .zip file"
        exit 1
    fi

    # Try to upload the bundle
    echo "Uploading ${zip_path}..."
    upload_action=$(curl -X POST https://microvisor-upload.twilio.com/v1/Apps -H "Content-Type: multipart/form-data" -u "${TWILIO_API_KEY}:${TWILIO_API_SECRET}" -s -F File=@"${zip_path}")

    app_sid=$(echo "${upload_action}" | jq -r '.sid')

    if [[ -z "${app_sid}" ]]; then
        echo "[ERROR] Could not upload app"
        exit 1
    else
        # Success... try to assign the app
        echo "Assigning app ${app_sid} to device ${MV_DEVICE_SID}..."
        update_action=$(curl -X POST "https://microvisor.twilio.com/v1/Devices/${MV_DEVICE_SID}" -u "${TWILIO_API_KEY}:${TWILIO_API_SECRET}" -s -d TargetApp="${app_sid}")
        up_date=$(echo "${update_action}" | jq -r '.date_updated')

        if [[ "${up_date}" != "null" ]]; then
            echo "Updated device ${MV_DEVICE_SID} @ ${up_date}"
        else
            echo "[ERROR] Could not assign app ${app_sid} to device ${MV_DEVICE_SID}"
            echo "Response from server:"
            echo "${update_action}"
            exit 1
        fi
    fi
fi

# Remove null file
rm -f null.d

# Dump remote debugging command if we can
echo -e "\nUse the following command to initiate remote debugging:"
if [[ "${public_key_path}" != "NONE" ]]; then
    echo "twilio microvisor:debug ${MV_DEVICE_SID} '${private_key_path}'"
else
    echo "twilio microvisor:debug ${MV_DEVICE_SID} '$(pwd)/${private_key_path}'"
fi

# Start logging if requested to do so
[[ ${do_log} -eq 1 ]] && stream_log
exit 0
