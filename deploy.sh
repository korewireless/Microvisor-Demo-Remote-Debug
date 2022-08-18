#!/usr/bin/env bash

#
# deploy
#
# Upload and deploy Microvisor application code
#
# @author    Tony Smith
# @copyright 2022, Twilio
# @version   2.0.0
# @license   MIT
#

# GLOBALS
app_dir=demo
app_name=mv-remote-debug-demo.zip
#------------^ APP SPECIFIC ^------------
private_key_path="build/${app_dir}/debug_auth_priv_key.pem"
cmake_path="${app_dir}/CMakeLists.txt"
zip_path="build/${app_dir}/${app_name}"
do_log=0
do_build=1
do_deploy=1
do_update=1
public_key_path=NONE
output_mode=text
mvplg_minor_min="3"
mvplg_patch_min="0"

# NOTE
# This script the build directory is called 'build' and exists within
# the project directory. If it is used to build the app, this will be
# the case. You can pass in alternative target for the build product
# eg. './deploy.sh build_alt/app/my_app.zip'

# FROM 1.6.0 -- Trap ctrl-c
stty -echoctl
trap 'echo Done' SIGINT

# FUNCTIONS
show_help() {
    echo -e "Usage:\n"
    echo -e "  deploy /optional/path/to/Microvisor/app/bunde.zip\n"
    echo -e "Options:\n"
    echo "  --log / -l            After deployment, start log streaming. Default: no logging"
    echo "  --output / -o {mode}  Log output mode: \'text\` or \`json\`"
    echo "  --public-key {path}   /path/to/remote/debugging/public/key.pem"
    echo "  --private-key {path}  /path/to/remote/debugging/private/key.pem"
    echo "  --deploy-only         Deploy without a build"
    echo "  --log-only            Start log streaming immediately; do not build or deploy"
    echo "  -h / --help           Show this help screen"
    echo
}

stream_log() {
    echo -e "\nLogging from ${MV_DEVICE_SID}..."
    if [[ ${output_mode} == "json" ]]; then
        twilio microvisor:logs:stream "${MV_DEVICE_SID}" --output=json | jq
    else
        twilio microvisor:logs:stream "${MV_DEVICE_SID}"
    fi
}

show_error_and_exit() {
    echo "[ERROR] $1... exiting"
    exit 1
}

check_prereqs() {
    #1: Bash version 4+
    bv=$(/usr/bin/env bash --version | grep 'GNU bash' | awk {'print $4'} | cut -d. -f1)
    [[ ${bv} -lt 4 ]] && show_error_and_exit "This script requires Bash 4+"

    #2: required utilities
    prqs=(jq cmake curl twilio)
    for prq in "${prqs[@]}"; do
        check=$(which ${prq}) || show_error_and_exit "Required utility ${prq} not installed"
    done
    
    #3: Twilio CLI 4.0.1
    info=$(twilio --version)
    info=$(echo $info | cut -d ' ' -f1 | cut -d '/' -f2)
    maj=$(echo $info | cut -d . -f1)
    min=$(echo $info | cut -d . -f2)
    pat=$(echo $info | cut -d . -f3)

    [[ $maj -lt 4 ]] && show_error_and_exit "Twilio CLI must be version 4.0.1 or above"
    [[ $maj -eq 4 && $min -eq 0 && $pat -eq 0 ]] && show_error_and_exit "Twilio CLI must be version 4.0.1 or above"

    #4: credentials set
    [[ -z ${TWILIO_ACCOUNT_SID} || -z ${TWILIO_AUTH_TOKEN} ]] && show_error_and_exit "Twilio credentials not set as environment variables"
    
    #5: Device SID set
    [[ -z ${MV_DEVICE_SID} ]] && show_error_and_exit "Twilio device SID not set as environment variable"
        
    #6: Microvisor plugin version
    result=$(twilio plugins | grep 'microvisor' | awk {'print $2'})
    minor=$(echo $result | cut -d. -f2)
    patch=$(echo $result | cut -d. -f3)
    [[ ${minor} -lt ${mvplg_minor_min} ]] && show_error_and_exit "Microvisor plugin 0.${mvplg_minor_min}.${mvplg_patch_min} or above required"
    [[ ${minor} -eq ${mvplg_minor_min} && ${patch} -lt ${mvplg_patch_min} ]] && show_error_and_exit "Microvisor plugin 0.${mvplg_minor_min}.${mvplg_patch_min} or above required"
}

build_app() {
    if [[ "${public_key_path}" != "NONE" ]]; then
        cmake -S . -B build -D "RD_PUBLIC_KEYPATH:STRING=${public_key_path}"
    else
        cmake -S . -B build
    fi

    if cmake --build build --clean-first > /dev/null ; then
        echo "App built"
    else
        show_error_and_exit "Could not build the app"
    fi
}

update_build_number() {
    build_val=$(grep 'set(BUILD_NUMBER "' "${cmake_path}")
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
# Check prequisites
check_prereqs

# Parse arguments
arg_is_value=0
last_arg=NONE
for arg in "$@"; do
    # Make arg lowercase
    check_arg=${arg,,}
    if [[ ${arg_is_value} -gt 0 ]]; then
        if [[ "${arg:0:1}" = "-" ]]; then
            show_error_and_exit "Missing value for ${last_arg}"
        fi
        case "${arg_is_value}" in
            1) private_key_path="${arg}" ; echo "Remote Debugging private key: ${private_key_path}" ;;
            2) public_key_path="${arg}"  ; echo "Remote Debugging public key: ${public_key_path}"   ;;
            3) output_mode="${check_arg}" ;;
            *) echo "[Error] Unknown argument" exit 1 ;;
        esac
        arg_is_value=0
        continue
    fi

    if [[ "${check_arg}" = "--log" || "${check_arg}" = "-l" ]]; then
        do_log=1
    elif [[ "${check_arg}" = "--private-key" ]]; then
        arg_is_value=1
        last_arg=${arg}
        continue
    elif [[ "${check_arg}" = "--public-key" ]]; then
        arg_is_value=2
        last_arg=${arg}
        continue
    elif [[ "${check_arg}" = "--output"  || "${check_arg}" = "-o" ]]; then
        arg_is_value=3
        last_arg=${arg}
        continue
    elif [[ "${check_arg}" = "--log-only" ]]; then
        do_log=1
        do_deploy=0
        do_build=0
    elif [[ "${check_arg}" = "--deploy-only" ]]; then
        do_build=0
    elif [[ "${check_arg}" = "--help" || "${check_arg}" = "-h" ]]; then
        show_help
        exit 0
    elif [[ "${arg:0:1}" = "-" ]]; then
        show_error_and_exit "Unknown command: ${arg}"
    else
        zip_path="${arg}"
    fi
done

# FROM 1.6.0 -- check output mode
if [[ ${output_mode} != "text" ]]; then
    if [[ ${output_mode} != "json" ]]; then
        output_mode=text
    fi
fi

if [[ ${do_build} -eq 1 ]]; then
    [[ ${do_update} -eq 1 ]] && update_build_number
    build_app
fi

if [[ ${do_deploy} -eq 1 ]]; then
    # Check we have what looks like a bundle
    extension="${zip_path##*.}"
    if [[ "${extension}" != "zip" ]]; then
        show_error_and_exit "${zip_path} does not indicate a .zip file"
    fi

    # Try to upload the bundle
    echo "Uploading ${zip_path}..."
    upload_action=$(curl -X POST https://microvisor-upload.twilio.com/v1/Apps -H "Content-Type: multipart/form-data" -u "${TWILIO_ACCOUNT_SID}:${TWILIO_AUTH_TOKEN}" -s -F File=@"${zip_path}")

    app_sid=$(echo "${upload_action}" | jq -r '.sid')

    if [[ -z "${app_sid}" || "${app_sid}" == "null" ]]; then
        show_error_and_exit "Could not upload app"
    else
        # Success... try to assign the app
        echo "Assigning app ${app_sid} to device ${MV_DEVICE_SID}..."
        if ! update_action=$(twilio api:microvisor:v1:devices:update ${MV_DEVICE_SID} --target-app=${app_sid}); then
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
