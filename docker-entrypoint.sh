#!/bin/bash
set -e

cd $(dirname $0)

if [ -d build ]; then
  rm -rf build
fi

cmake -S . -B build
cmake --build build
cd build

twilio microvisor:debug:generate_keypair \
  --debug-auth-privkey prv-key.pem \
  --debug-auth-pubkey  pub-key.pem

twilio microvisor:apps:bundle  \
  demo/mv-remote-debug-demo.bin \
  demo/mv-remote-debug-demo.zip

unique_name=$(uuidgen)

if twilio microvisor:apps:create demo/mv-remote-debug-demo.zip $unique_name ; then
    if twilio api:microvisor:v1:devices:update --sid ${MV_DEVICE_SID} --target-app ${unique_name} ; then
        echo App name is ${unique_name}
        echo "Private key written to $(pwd)/prv-key.pem"
        echo " Public key written to $(pwd)/pub-key.pem"
        twilio microvisor:debug ${MV_DEVICE_SID} '${private_key_path}
        echo "In the terminal window that opens, run 'gdb-multiarch -l 10 ./build/demo/mv-remote-debug-demo.elf'"
    fi
fi

