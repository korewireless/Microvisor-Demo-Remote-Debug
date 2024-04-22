# Microvisor Remote Debugging Demo

This repo provides a basic user application that you can use to try out Microvisor’s remote debugging feature.

The application code files can be found in the [demo/](demo/) directory. The [ST_Code/](ST_Code/) directory contains required components that are not part of the Microvisor STM32U5 HAL, which this sample accesses as a submodule.

This repo contains a `.gdbinit` file which sets the remote target to localhost on port 8001 to match the Twilio CLI Microvisor plugin remote debugging defaults. To enable this file, add `set auto-load safe-path .` to your `~/.gdbinit` file, creating one if necessary.

It also contains a `.vscode` directory containing configuration files to support running the demo using [Visual Studio Code](https://code.visualstudio.com/). This mode requires the following plugins to be installed into VSCode:

* [Native Debug](https://marketplace.visualstudio.com/items?itemName=webfreak.debug)
* [Cmake](https://marketplace.visualstudio.com/items?itemName=twxs.cmake)
* [Cmake Tools](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cmake-tools)
* [C/C++](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools)

## Cloning the Repo

This repo makes uses of git submodules, some of which are nested within other submodules. To clone the repo, run:

```bash
git clone --recurse-submodules https://github.com/korewireless/Microvisor-Demo-Remote-Debug.git
```

and then:

```bash
cd Microvisor-Demo-Remote-Debug
```

## Requirements

You will need a Twilio account. [Sign up now if you don’t have one](https://www.twilio.com/try-twilio).

You will also need a Microvisor Nucleo Development Board.

## Software Setup

This project is written in C. At this time, we only support Ubuntu 20.0.4. Users of other operating systems should build the code under a virtual machine running Ubuntu, or with Docker.

**Note** Users of unsupported platforms may attempt to install the Microvisor toolchain using [this guidance](https://www.twilio.com/docs/iot/microvisor/install-microvisor-app-development-tools-on-unsupported-platforms).

### Docker

If you are running on an architecture other than x86/amd64 (such as a Mac with Apple silicon), you will need to override the platform when running docker. This is needed for the Twilio CLI `apt` package which is x86 only at this time:

```shell
export DOCKER_DEFAULT_PLATFORM=linux/amd64
```

Build the image:

```shell
docker build --build-arg UID=$(id -u) --build-arg GID=$(id -g) -t mv-rd-demo-image .
```

Run the build:

```shell
docker run -it --rm -v $(pwd)/:/home/mvisor/project/ \
  --env-file env.list --name mv-rd-demo mv-rd-demo-image
```

**Note** You will need to have exported certain environment variables, as [detailed below](#environment-variables).

Under Docker, the demo is compiled, uploaded and deployed to your development board. The GDB server is started, and then so is GDB.

### Libraries and Tools

Under Ubuntu, run the following:

```bash
sudo apt install gcc-arm-none-eabi binutils-arm-none-eabi git curl \
  build-essential cmake libsecret-1-dev jq openssl gdb-multiarch
```

### Twilio CLI

Install the Twilio CLI. This is required to view streamed logs and for remote debugging. You need version 4.0.1 or above.

**Note** If you have already installed the Twilio CLI using *npm*, we recommend removing it and then reinstalling as outlined below. Remove the old version with `npm remove -g twilio-cli`.

```bash
wget -qO- https://twilio-cli-prod.s3.amazonaws.com/twilio_pub.asc | sudo apt-key add -
sudo touch /etc/apt/sources.list.d/twilio.list
echo 'deb https://twilio-cli-prod.s3.amazonaws.com/apt/ /' | sudo tee /etc/apt/sources.list.d/twilio.list
sudo apt update
sudo apt install -y twilio
```

Close your terminal window or tab, and open a new one. Now run:

```bash
twilio plugins:install @twilio/plugin-microvisor
```

#### Environment Variables

Running the Twilio CLI and the Microvisor Plugin to upload the built code to the Microvisor cloud for subsequent deployment to your Microvisor Nucleo Board uses the following Twilio credentials stored as environment variables. They should be added to your shell profile:

```bash
export TWILIO_ACCOUNT_SID=ACxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
export TWILIO_AUTH_TOKEN=xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
export MV_DEVICE_SID=UVxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
```

You can get the first two from your Twilio Console [account dashboard](https://console.twilio.com/).

The third value can be found in the [**Iot > Microvisor > Devices** section](https://console.twilio.com/us1/develop/iot/microvisor/devices). It is also accessible via the QR code on the back of your development board. Scan the code with your mobile phone and a suitable app, and the board’s SID is the third /-separated field.

## Build and deploy the application

Run:

```bash
twilio microvisor:deploy . --devicesid ${MV_DEVICE_SID} --log --genkeys
```

This will build the demo, upload the build, and stage it for deployment to your device. If you encounter errors, please check your stored Twilio credentials.

The `--log` flag initiates log-streaming.

Watch the log for the lines

```shell
INSTALLING_APP
STARTING_NEW_APP
COMPLETE
```

which indicate the version of the application with your remote debugging public key is now running on the device and ready for remote debugging.

## UART Logging

You may log your application over UART on pin PD5 — pin 41 in bank CN11 on the Microvisor Nucleo Development Board. To use this mode, which is intended as an alternative to application logging, typically when a device is disconnected, connect a 3V3 FTDI USB-to-Serial adapter cable’s RX pin to PD5, and a GND pin to any Nucleo GND pin. Whether you do this or not, the application will continue to log via the Internet.

## VSCode Debugging

1. Open the VSCode workspace file `mv-remote-debug-demo.code-workspace`.
1. Select  `GCC 10.3.1 arm-none-eabi` (your version number may be different) from the CMake **No active kit** button on the bar at the bottom of the window if it is not already selected.
1. Click on the **Run and Debug** icon in the left-hand toolbar.
1. Click on **Microvisor Remote** at the top of the **RUN AND DEBUG** column on the left.

## A short GDB tutorial

This repo’s code was written to provide a basic GDB usage demo.

Open a new terminal window or tab and run:

```
twilio microvisor:debug $MV_DEVICE_SID build/demo/debug_auth_priv_key.pem
```

Now open a third terminal window or tab, navigate to the repo directory, and start GDB:

```
gdb-multiarch ./build/demo/*.elf
```

The app will stop, allowing you to enter a breakpoint, as follows:

```
(gdb) b main.c:82
```

Enter `c` to continue running the code. When the breakpoint trips, you will see:

```
Breakpoint 1, main () at /home/smitty/GitHub/microvisor-remote-debug-demo/demo/main.c:82
82	            debug_function_parent(&store);
```

Print the value of the `store` variable:

```
(gdb) p store
$3 = 42
```

Now step into the function, `debug_function_parent()`, with the `s` command. You will move to the function’s first line:

```
debug_function_parent (vptr=0x2007ffac) at /home/smitty/GitHub/microvisor-remote-debug-demo/demo/main.c:143
143	    uint32_t test_var = *vptr;
```

Use `n` to move to the next line:

```
144	    bool result = debug_function_child(&test_var);
```

Use `n` again to step over `debug_function_child()`:

```
145	    *vptr = test_var;
```

Print the value of `test_var`:

```
(gdb) p test_var
$3 = 43
```

Use `n` to step through the function’s remaining lines until you come to the function’s closing brace (`}`). Now enter `fin` to continue running, to return from the function, and to stop at the next line in the calling function:

```
146	}
(gdb) fin
Run till exit from #0  debug_function_parent (vptr=0x2007ffac) at /home/smitty/GitHub/microvisor-remote-debug-demo/demo/main.c:146
main () at /home/smitty/GitHub/microvisor-remote-debug-demo/demo/main.c:83
83	            server_log("Debug test variable value: %lu\n", store);
```

Now print `store` again:

```
(gdb) p store
$3 = 43
```

It has been successfully incremented — albeit via a rather circuitous route.

Enter `c` to continue. When GDB halts again, at the breakpoint, once more step into `debug_function_parent()` and step line by line until you come to the next function call. This time step into the function `debug_function_child()` with the `s` command. Now enter `bt` to get a backtrace. This lists the functions through which you have passed to reach this point:

```
(gdb) bt
#0  debug_function_child (vptr=0x2007ff9c) at /home/smitty/GitHub/microvisor-remote-debug-demo/demo/main.c:150
#1  0x0800c274 in debug_function_parent (vptr=0x2007ffac) at /home/smitty/GitHub/microvisor-remote-debug-demo/demo/main.c:144
#2  0x0800c130 in main () at /home/smitty/GitHub/microvisor-remote-debug-demo/demo/main.c:82
```

You can use the `fin` command to run the rest of the current function and have GDB halt again after the function has returned:

```
(gdb) fin
Run till exit from #0  debug_function_child (vptr=0x2007ff9c) at /home/smitty/GitHub/microvisor-remote-debug-demo/demo/main.c:150
debug_function_parent (vptr=0x2007ffac) at /home/smitty/GitHub/microvisor-remote-debug-demo/demo/main.c:145
145	    *vptr = test_var;
Value returned is $2 = true
```

Over to you. Use the GDB tools you’ve just demo’d to add some more breakpoints to the code, and step through some of the other parts of the application. To get a list of breakpoints at any time, enter `info breakpoints`.

For more guidance on making use of GDB, see the guide [**Microvisor Remote Debugging**](https://www.twilio.com/docs/iot/microvisor/microvisor-remote-debugging). It also covers Visual Studio Code usage.

## Copyright and Licensing

The sample code and Microvisor SDK is © 2024, KORE Wireless. It is licensed under the terms of the [MIT License](./LICENSE.md).

The SDK makes used of code © 2022, STMicroelectronics and affiliates. This code is licensed under terms described in [this file](https://github.com/twilio/twilio-microvisor-hal-stm32u5/blob/main/LICENSE-STM32CubeU5.md).
