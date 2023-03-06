# Docker Files for OpenBK7231T_App

This docker will build all or some of the platforms that OpenBeken supports. To use, first build the docker image:

```sh
docker build -t openbk_build --build-arg USERNAME=$USER .
```

Note that the current user name is passed through to the docker image build. This is to preserve local file permissions when OpenBeken is built.

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

  For example, to build `OpenBK7231T` and `OpenXR809`, the options should be `-env TARGET_SDKS="OpenBK7231T,OpenXR809"`
* `MAKEFLAGS` - This is the standard `make` environment variable for setting default `make` flags. It is recommended to use this to configure `make` to use multiple cores. for exmaple, to use 8 cores: `--env MAKEFLAGS="-j 8"`
