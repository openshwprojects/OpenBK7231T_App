# Docker Files for OpenBK7231T_App

## Build Environment
The `Dockerfile.build_env` file can be used to create a consistent Linux-based environment for building OpenBK7231T_App. This allows you to build OpenBK7231T_App on any flavor of Linux and MacOS. This should also work on Windows, but is untested.

To build the image, run from within this directory:
```sh
docker build -t openbk7231t_build --build-arg USERNAME=$USER -f Dockerfile.build_env .
```

The `cd` to the root of the OpenBK7231T SDK repo that has been set up [as described in the build instructions](https://github.com/michaelkamprath/OpenBK7231T_App/blob/main/BUILDING.md#building-for-bk7231t), and then start a shell prompt in the built Docker image, that binds the SDK repo as a volume in the Docker image:

```sh
docker run -it -v "$(pwd)/":/home/$USER/OpenBK7231T --user $USER  openbk7231t_build /bin/bash
```
Finally, at the Docker image's shell prompt, execute the following to build:
```sh
cd ~/OpenBK7231T/
./b.sh
```
The last command may be changed to the advanced build's `build_app.sh` command as described in [the build instructions](https://github.com/michaelkamprath/OpenBK7231T_App/blob/main/BUILDING.md#building-for-bk7231t). The build results will be placed into the host computer's file system as described in the build instructions. To exit the Docker image's shell prompt when the build is done, use the `exit` command.
