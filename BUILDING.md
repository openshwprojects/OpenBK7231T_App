# Setup

## Editor

Any editor can be used for editing. However, the repository does have settings to ensure consistent formatting when using [Visual Code](https://code.visualstudio.com/). For other editors, please use `tab` instead of `spaces` with `tabSize` of `4` for indentation.

## Nodejs

You would need `nodejs` if you plan on making JavaScript/styling changes. This is to run `gulp` tasks and more things in future.

- Install [nodejs](https://nodejs.org/en/).
- Run `npm install` at the root folder to install dev packages.

Gulp tasks should automatically appear in Explore pane in Visual Code. They can also be invoked from console by running `gulp`.

# Building
A Docker-ized build environment can be used to perform these builds. Instructions for doing so are [available here](./docker/README.md).

## Building for BK7231T

Get the SDK repo:
https://github.com/openshwprojects/OpenBK7231T
Clone it to a folder, e.g. `bk7231sdk/`

Clone the [app](https://github.com/openshwprojects/OpenBK7231T_App) repo into `bk7231sdk/apps/<appname>` - e.g. `bk7231sdk/apps/openbk7231app`.

On Windows, install [Cygwin](https://www.cygwin.com). Manually search for and install the "make" and "python3" packages during the setup. Note that Cygwin must be installed in a directory without whitespaces in the path.

Open Cygwin and browse to the SDK repo folder (`cd /cygdrive/c/Users/<path to folder>`).

Build using:
`./b.sh`

You can also do advanced builds using `build_app.sh`:
`./build_app.sh apps/<appname> <appname> <appversion>`
(appname must be identical to foldername in `apps/` folder)

e.g. `./build_app.sh apps/openbk7231app openbk7231app 1.0.0`.

The output binaries can be found at `apps/<appname>/output/<appversion>`.

## Building for BK7231N

Same as for BK7231T, but use BK7231N SDK and you also might need to rename project directory from OpenBK7231T_App to OpenBK7231N_App:
https://github.com/openshwprojects/OpenBK7231N

## Building for XR809

Get XR809 SDK:
https://github.com/openshwprojects/OpenXR809

Checkout [this repository](https://github.com/openshwprojects/OpenBK7231T_App) to openxr809/project/oxr_sharedApp/shared/

Run ./build_oxr_sharedapp.sh

## Building for BL602

Get the SDK repo:
https://github.com/openshwprojects/OpenBL602
Clone it to a folder, e.g. `OpenBL602/`

Clone the [app](https://github.com/openshwprojects/OpenBK7231T_App) repo into `OpenBL602/customer_app/bl602_sharedApp/bl602_sharedApp/shared` (such that the `.git` folder is placed in the `shared` folder).

On Windows, install [MSys2](https://www.msys2.org/) and open a Msys2 terminal. Install `make` using `pacman -S make`.
Create a copy of the `OpenBL602/toolchain/riscv/MSYS` folder and rename it to `OpenBL602/toolchain/riscv/MINGW64`.

Open a Msys2 terminal and browse to the `OpenBL602/customer_app/bl602_sharedApp` folder.

Build using:
`./genromap`

The output binaries can be found at `OpenBL602/customer_app/bl602_sharedApp/build_out/bl602_sharedApp.bin`.

## Building for W600

https://github.com/openshwprojects/OpenW600

## Building for W800/W801

To build for W800, you need our W800 SDK fork:

https://github.com/openshwprojects/OpenW800

also checkout this repository (OpenBK7231T_App), put into the shared app directory in the SDK, so you get paths like:

OpenW800\sharedAppContainer\sharedApp\src\devicegroups

then, to compile, you only need C-Sky Development Suite for CK-CPU C/C++ Developers (V5.2.11 B20220512)
Get it from here (you'd need to register):

https://occ.t-head.cn/community/download

The IDE/compiler bundle I used was: cds-windows-mingw-elf_tools-V5.2.11-20220512-2012.zip

  
# Building for Windows
  
It is also possible to build OpenBeken for Windows. Entire OBK builds correctly, along with script support and full MQTT support, but there is a minor issue in Winsock code which breaks Tasmota Control compatibility. To build for Windows, open openBeken_win32_mvsc2017 in Microsoft Visual Studio Community 2017 and select configuration Debug Windows or Debug Windows Scriptonly and press build.
This should make development and testing easier.
LittleFS works in Windows build, it operates on 2MB memory saved in file, so you can even test scripting, etc
