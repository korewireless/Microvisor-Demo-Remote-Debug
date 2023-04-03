#!/bin/bash
set -e

cd $(dirname $0)

[ -d build ] && rm -rf build

if [[ -n "${MICROVISOR_GITHUB_ACTION_REMOTE_DEBUG_TEST}" ]]; then
  # Build only
  twilio microvisor:deploy . -b
else
  unique_name=$(uuidgen)
  twilio microvisor:deploy . --genkeys --devicesid ${MV_DEVICE_SID} --build
  
  if twilio microvisor:apps:create build/demo/project.zip $unique_name ; then
    if twilio api:microvisor:v1:devices:update --sid ${MV_DEVICE_SID} --target-app ${unique_name} ; then
        echo App name is ${unique_name}
        echo Starting GDB server...
        sleep 10
        twilio microvisor:debug ${MV_DEVICE_SID} build/debug_auth_prv_key.pem &
        
        echo Starting GDB...
        sleep 10
        gdb-multiarch -l 10 --eval-command="target remote localhost:8001" build/demo/mv-remote-debug-demo.elf
    fi
  fi
fi
