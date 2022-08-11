

# Building for BK7231T

Get the SDK repo:
https://github.com/openshwprojects/OpenBK7231T
Clone it to a folder, e.g. bk7231sdk/

Clone the [app](https://github.com/openshwprojects/OpenBK7231T_App) repo into bk7231sdk/apps/<appname> - e.g. bk7231sdk\apps\openbk7231app

On Windows, start a cygwin prompt.

go to the SDK folder.

build using:
`./b.sh`
  
you can also do advanced build by build_app.sh:
`./build_app.sh apps/<appname> <appname> <appversion>`
(appname must be identical to foldername in apps/ folder)
  
e.g. `./build_app.sh apps/openbk7231app openbk7231app 1.0.0`
  
  
 
# Building for BK7231N

Same as for BK7231T, but use BK7231N SDK and you also might need to rename project directory from OpenBK7231T_App to OpenBK7231N_App:
https://github.com/openshwprojects/OpenBK7231N
  
   
# Building for XR809

Get XR809 SDK:
https://github.com/openshwprojects/OpenXR809

Checkout [this repository](https://github.com/openshwprojects/OpenBK7231T_App) to openxr809/project/oxr_sharedApp/shared/
  
Run ./build_oxr_sharedapp.sh
  
    
# Building for W800/W801

To build for W800, you need our W800 SDK fork:

https://github.com/openshwprojects/OpenW800

also checkout this repository (OpenBK7231T_App), put into the shared app directory in the SDK, so you get paths like:

OpenW800\sharedAppContainer\sharedApp\src\devicegroups

then, to compile, you only need C-Sky Development Suite for CK-CPU C/C++ Developers (V5.2.11 B20220512)
Get it from here (you'd need to register):

https://occ.t-head.cn/community/download

The IDE/compiler bundle I used was: cds-windows-mingw-elf_tools-V5.2.11-20220512-2012.zip

  
  
  
  
  
  
  
  
  
  
