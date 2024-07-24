#!/bin/bash
set -e

cd $(dirname $0)

rm -rf build

# Place these here so dependencies are checked
# even if the take the BUILD_ONLY path (GitHub Action)
unique_name=$(uuidgen)
which gdb-multiarch > /dev/null || exit 1

if [[ -n "${BUILD_ONLY}" ]]; then
  # Build only
  twilio microvisor:deploy . -b
else
  # Build but don't deploy
  twilio microvisor:deploy . --genkeys --build

  # Upload build with unique app name
  if twilio microvisor:apps:create build/demo/project.zip $unique_name ; then
    # Deploy unique app name to device
    if twilio api:microvisor:v1:devices:update --sid ${MV_DEVICE_SID} --target-app ${unique_name} ; then
        # Start debug server in background
        echo App name is ${unique_name}
        echo Starting GDB server...
        sleep 10
        twilio microvisor:debug ${MV_DEVICE_SID} build/debug_auth_prv_key.pem &

        # Start debugger
        echo Starting GDB...
        sleep 10
        gdb-multiarch -l 10 --eval-command="target remote localhost:8001" build/demo/mv-remote-debug-demo.elf
    fi
  fi
fi
