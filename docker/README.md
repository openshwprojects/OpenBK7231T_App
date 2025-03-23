# Docker Files for OpenBK7231T_App

This docker will build all or some of the platforms that OpenBeken supports. To use, first build the docker image:

```sh
docker build -t openbk_build --build-arg UID=$UID --build-arg USERNAME=$USER .
```

Note that the current user name and user ID is passed through to the docker image build.
This is to preserve local file permissions when OpenBeken is built.

If you want to change the timezone you can use the argument TZ for that like

```sh
docker build -t openbk_build --build-arg UID=$UID --build-arg USERNAME=$USER --build-arg TZ="Etc/UTC" .
```

or

```sh
docker build -t openbk_build --build-arg UID=$UID --build-arg USERNAME=$USER --build-arg TZ="Asia/Tokyo" .
```


Once the docker image is build, you can use it to build the SDKs as follows (assumed you are in the current `docker` directory):

```sh
docker run -it -v "$(pwd)/..":/OpenBK7231T_App  openbk_build
```

When complete, all target platform builds with be present in the repository's `output` directory.

When running the docker image, you may pass the following environment variables with the `--env` option:

* `APP_VERSION` - The version identifier for the build. If not provided, a default version will be used based on the time of the build.
* `TARGET_SDKS` - A comma-separated list of platforms (with no spaces) that should be built. If not present, all platforms will be built. The supported platform identifiers are:
  * `OpenBK7231T`
  * `OpenBK7231N`
  * `OpenXR809`
  * `OpenBL602`
  * `OpenW800`
  * `OpenW600`
  * `OpenLN882H`

  For example, to build `OpenBK7231T` and `OpenXR809`, the options should be `-env TARGET_SDKS="OpenBK7231T,OpenXR809"`
* `MAKEFLAGS` - This is the standard `make` environment variable for setting default `make` flags. It is recommended to use this to configure `make` to use multiple cores. for exmaple, to use 8 cores: `--env MAKEFLAGS="-j 8"`

## Building on a Apple Silicon Macs
The software that the build process uses to compile the binaries is for the `x86` platform and can only be run by `x86` CPUs. Fortunately, this does not mean you cannot build OpenBeken on Apple Silicon (ARM) Macintosh computers. Docker is able to run `x86` docker images on Apple Silicon Macs through the Rosetta emulator. To do this, you need to enable running `x86` images in docker by following [the instructions outlined in this article](https://blog.jaimyn.dev/how-to-build-multi-architecture-docker-images-on-an-m1-mac/) (or [this article](https://levelup.gitconnected.com/docker-on-apple-silicon-mac-how-to-run-x86-containers-with-rosetta-2-4a679913a0d5)).

Once you have set up your Apple Silicon Mac to run `x86` docker images, build the OpenBeken build environment docker image with this command from within the `docker` directory of this repository:
```sh
docker buildx build --platform linux/amd64 --load -t openbk_build --build-arg USERNAME=$USER .
```

Once the docker image is built, you can run the OpenBeken build with this instruction:
```sh
docker run --platform linux/amd64 -it -v "$(pwd)/..":/OpenBK7231T_App  openbk_build
```

All the same environment variable options described above work here too. Sometimes builds will not be successful using this approach as `x86` emulation can have occasional glitches. Retrying the build for the OpenBeken platforms that failed usually works on the second try.
