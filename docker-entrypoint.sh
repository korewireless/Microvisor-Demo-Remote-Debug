#!/bin/bash
set -e

cd $(dirname $0)

[ -d build ] && rm -rf build

if [[ -n "${MICROVISOR_GITHUB_ACTION_REMOTE_DEBUG_TEST}" ]]; then
  # Build only
  twilio microvisor:deploy . -b
else
  # Build and deploy -- requires env vars for device SID and Twilio creds to be set
  twilio microvisor:deploy . --devicesid ${MV_DEVICE_SID} --genkeys --privatekey prv-key.pem --publickey pub-key.pem
  twilio microvisor:debug ${MV_DEVICE_SID} prv-key.pem
  echo "In the terminal window that opens, run 'gdb-multiarch -l 10 ./build/demo/mv-remote-debug-demo.elf'"
fi
