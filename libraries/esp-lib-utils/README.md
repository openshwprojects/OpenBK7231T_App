[![Arduino Lint](https://github.com/esp-arduino-libs/esp-lib-utils/actions/workflows/arduino_lint.yml/badge.svg)](https://github.com/esp-arduino-libs/esp-lib-utils/actions/workflows/arduino_lint.yml) [![pre-commit](https://github.com/esp-arduino-libs/esp-lib-utils/actions/workflows/pre-commit.yml/badge.svg)](https://github.com/esp-arduino-libs/esp-lib-utils/actions/workflows/pre-commit.yml) [![Build Test Apps](https://github.com/esp-arduino-libs/esp-lib-utils/actions/workflows/build_test.yml/badge.svg)](https://github.com/esp-arduino-libs/esp-lib-utils/actions/workflows/build_test.yml) [![Version Consistency](https://github.com/esp-arduino-libs/esp-lib-utils/actions/workflows/check_lib_versions.yml/badge.svg)](https://github.com/esp-arduino-libs/esp-lib-utils/actions/workflows/check_lib_versions.yml)

**Latest Arduino Library Version**: [![GitHub Release](https://img.shields.io/github/v/release/esp-arduino-libs/esp-lib-utils)](https://github.com/esp-arduino-libs/esp-lib-utils/releases)

**Latest Espressif Component Version**: [![Espressif Release](https://components.espressif.com/components/espressif/esp-lib-utils/badge.svg)](https://components.espressif.com/components/espressif/esp-lib-utils)

# ESP Library Utils

## Overview

`esp-lib-utils` is a library designed to offer utility functions for other libraries, including the following features:

- Supports a variety of utility functions, such as `logging`, `checking`, and `memory` functions.
- Supports flexible static configuration, such as setting log levels, enabling/disabling checks, using custom memory allocation functions, and more.
- Compatible with the `Arduino`, `ESP-IDF` and `MicroPython` for compilation.

## Table of Contents

- [ESP Library Utils](#esp-library-utils)
  - [Overview](#overview)
  - [Table of Contents](#table-of-contents)
  - [How to Use](#how-to-use)
    - [ESP-IDF Framework](#esp-idf-framework)
      - [Dependencies and Versions](#dependencies-and-versions)
      - [Adding to Project](#adding-to-project)
      - [Configuration Instructions](#configuration-instructions)
    - [Arduino IDE](#arduino-ide)
      - [Dependencies and Versions](#dependencies-and-versions-1)
      - [Installing the Library](#installing-the-library)
      - [Configuration Instructions](#configuration-instructions-1)
  - [Usage Example](#usage-example)
    - [Logging Functions](#logging-functions)
    - [Checking Functions](#checking-functions)
    - [Memory Functions](#memory-functions)
  - [FAQ](#faq)
    - [Where is the directory for Arduino libraries?](#where-is-the-directory-for-arduino-libraries)
    - [How to Install esp-lib-utils in Arduino IDE?](#how-to-install-esp-lib-utils-in-arduino-ide)
    - [Where are the installation directory for arduino-esp32 and the SDK located?](#where-are-the-installation-directory-for-arduino-esp32-and-the-sdk-located)

## How to Use

### ESP-IDF Framework

#### Dependencies and Versions

|                 **Dependency**                  | **Version** |
| ----------------------------------------------- | ----------- |
| [esp-idf](https://github.com/espressif/esp-idf) | >= 5.1      |

#### Adding to Project

`esp-lib-utils` has been uploaded to the [Espressif Component Registry](https://components.espressif.com/), and users can add it to their project using the `idf.py add-dependency` command, for example:

```bash
idf.py add-dependency "espressif/esp-lib-utils"
```

Alternatively, users can create or modify the *idf_component.yml* file in the project directory. For more details, please refer to the [Espressif Documentation - IDF Component Manager](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/tools/idf-component-manager.html).

#### Configuration Instructions

When developing with esp-idf, users can configure `esp-lib-utils` through the menuconfig:

1. Run the command `idf.py menuconfig`.
2. Navigate to `Component config` > `ESP Library Utils Configurations`.

> [!NOTE]
> To support `try-catch` when using `esp_utils::make_shared()` in C++, users need to enable the `COMPILER_CXX_EXCEPTIONS` configuration in the menuconfig.

### Arduino IDE

#### Dependencies and Versions

|                       **Dependency**                        | **Version** |
| ----------------------------------------------------------- | ----------- |
| [arduino-esp32](https://github.com/espressif/arduino-esp32) | >= v3.0.0   |

#### Installing the Library

For installation of the `esp-lib-utils` library, refer to [How to Install esp-lib-utils in Arduino IDE](#how-to-install-esp-lib-utils-in-arduino-ide).

#### Configuration Instructions

Below are detailed instructions on how to configure `esp-lib-utils`. They are optional operations and are configured through the [esp_utils_conf.h](../esp_utils_conf.h) file with the following characteristics:

1. The path sequence to search for the *esp_utils_conf.h* file is: `Current Project Directory` > `Arduino Library Directory` > `esp-lib-utils Directory`.
3. **If a project needs to use a separate configuration**, users can copy it from the root directory of `esp-lib-utils` to the project.
4. **If multiple projects need to use the same configuration**, users can place it in the [Arduino Library Directory](#where-is-the-directory-for-arduino-libraries), so that all projects can share the same configuration. The layout of the Arduino library folder should look like this:

   ```
   Arduino
       |-libraries
           |-esp-lib-utils
           |-esp_utils_conf.h
           |-other_libs
   ```

> [!WARNING]
> * When the library version is updated, the configurations within the *esp_utils_conf.h* file might change, such as adding, deleting, or renaming.
> * To ensure the compatibility of the program, the library manages the version of the file independently and checks whether the configuration file currently used by the user are compatible with the library during compilation.
> * Detailed version information and checking rules can be found at the end of the file.

Since `esp-lib-utils` configures its functionality through the *esp_utils_conf.h* file, users can customize behavior and default parameters by adjusting the macro definitions within this file. For example, to enable debug log output, here is a snippet of the modified `esp_utils_conf.h` file:

```c
...
/**
 * Global log level, logs with a level lower than this will not be compiled. Choose one of the following:
 *  - ESP_UTILS_LOG_LEVEL_DEBUG:   Extra information which is not necessary for normal use (values, pointers, sizes, etc)
 *                                 (lowest level)
 *  - ESP_UTILS_LOG_LEVEL_INFO:    Information messages which describe the normal flow of events
 *  - ESP_UTILS_LOG_LEVEL_WARNING: Error conditions from which recovery measures have been taken
 *  - ESP_UTILS_LOG_LEVEL_ERROR:   Critical errors, software module cannot recover on its own
 *  - ESP_UTILS_LOG_LEVEL_NONE:    No log output (highest level) (Minimum code size)
 */
#define ESP_UTILS_CONF_LOG_LEVEL            (ESP_UTILS_LOG_LEVEL_DEBUG)
...
```

> [!NOTE]
> `arduino-esp32` supports `try-catch` in C++ by default.

## Usage Example

### Logging Functions

```cpp
// Define the log tag for the current library, should be declared before `esp_lib_utils.h`
#define ESP_UTILS_LOG_TAG "MyLibrary"
#include "esp_lib_utils.h"

void test_log(void)
{
    ESP_UTILS_LOG_TRACE_ENTER();

    ESP_UTILS_LOGD("This is a debug message");
    ESP_UTILS_LOGI("This is an info message");
    ESP_UTILS_LOGW("This is a warning message");
    ESP_UTILS_LOGE("This is an error message");

    ESP_UTILS_LOG_TRACE_EXIT();
}
```

### Checking Functions

```cpp
// Define the log tag for the current library, should be declared before `esp_lib_utils.h`
#define ESP_UTILS_LOG_TAG "MyLibrary"
#include "esp_lib_utils.h"

bool test_check_false_return(void)
{
    ESP_UTILS_CHECK_FALSE_RETURN(true, false, "Check false return failed");
    ESP_UTILS_CHECK_FALSE_RETURN(false, true, "Check false return success");
    return false;
}

bool test_check_false_goto(void)
{
    ESP_UTILS_CHECK_FALSE_GOTO(true, err, "Check false goto failed");
    ESP_UTILS_CHECK_FALSE_GOTO(false, end, "Check false goto success");

err:
    return false;

end:
    return true;
}

void test_check_false_exit(void)
{
    ESP_UTILS_CHECK_FALSE_EXIT(true, "Check false exit failed");
    ESP_UTILS_CHECK_FALSE_EXIT(false, "Check false exit success");
}

bool test_check_error_return(void)
{
    ESP_UTILS_CHECK_ERROR_RETURN(ESP_OK, false, "Check error return failed");
    ESP_UTILS_CHECK_ERROR_RETURN(ESP_FAIL, true, "Check error return success");
    return false;
}

bool test_check_error_goto(void)
{
    ESP_UTILS_CHECK_ERROR_GOTO(ESP_OK, err, "Check error goto failed");
    ESP_UTILS_CHECK_ERROR_GOTO(ESP_FAIL, end, "Check error goto success");

err:
    return false;

end:
    return true;
}

void test_check_error_exit(void)
{
    ESP_UTILS_CHECK_ERROR_EXIT(ESP_OK, "Check error exit failed");
    ESP_UTILS_CHECK_ERROR_EXIT(ESP_FAIL, "Check error exit success");
}

bool test_check_null_return(void)
{
    ESP_UTILS_CHECK_NULL_RETURN((void *)1, false, "Check null return failed");
    ESP_UTILS_CHECK_NULL_RETURN(NULL, true, "Check null return success");
    return false;
}

static bool test_check_null_goto(void)
{
    ESP_UTILS_CHECK_NULL_GOTO((void *)1, err, "Check null goto failed");
    ESP_UTILS_CHECK_NULL_GOTO(NULL, end, "Check null goto success");

err:
    return false;

end:
    return true;
}

void test_check_null_exit(void)
{
    ESP_UTILS_CHECK_NULL_EXIT((void *)1, "Check null exit failed");
    ESP_UTILS_CHECK_NULL_EXIT(NULL, "Check null exit success");
}

```

### Memory Functions

```cpp
// Define the log tag for the current library, should be declared before `esp_lib_utils.h`
#define ESP_UTILS_LOG_TAG "MyLibrary"
#include "esp_lib_utils.h"

template <typename T, typename... Args>
std::shared_ptr<T> make_shared(Args &&... args)
{
    return std::allocate_shared<T, esp_utils::GeneralMemoryAllocator<T>>(
               esp_utils::GeneralMemoryAllocator<T>(), std::forward<Args>(args)...
           );
}

void test_memory(void)
{
    // Allocate memory in C style (`malloc/calloc` and `free` are re-defined in the library)
    int *c_ptr = (int *)malloc(sizeof(int));
    ESP_UTILS_CHECK_NULL_EXIT(c_ptr, "Failed to allocate memory");
    free(c_ptr);

    // Allocate memory in C++ style
    std::shared_ptr<int> cxx_ptr = nullptr;
    ESP_UTILS_CHECK_EXCEPTION_EXIT(
        (cxx_ptr = make_shared<int>()), "Failed to allocate memory"
    );
    cxx_ptr = nullptr;
}
```

## FAQ

### Where is the directory for Arduino libraries?

Users can find and modify the directory path for Arduino libraries by selecting `File` > `Preferences` > `Settings` > `Sketchbook location` from the menu bar in the Arduino IDE.

### How to Install esp-lib-utils in Arduino IDE?

- **If users want to install online**, navigate to `Sketch` > `Include Library` > `Manage Libraries...` in the Arduino IDE, then search for `esp-lib-utils` and click the `Install` button to install it.
- **If users want to install manually**, download the required version of the `.zip` file from [esp-lib-utils](https://github.com/esp-arduino-libs/esp-lib-utils), then navigate to `Sketch` > `Include Library` > `Add .ZIP Library...` in the Arduino IDE, select the downloaded `.zip` file, and click `Open` to install it.
- Users can also refer to the guides on library installation in the [Arduino IDE v1.x.x](https://docs.arduino.cc/software/ide-v1/tutorials/installing-libraries) or [Arduino IDE v2.x.x](https://docs.arduino.cc/software/ide-v2/tutorials/ide-v2-installing-a-library) documentation.

### Where are the installation directory for arduino-esp32 and the SDK located?

The default installation path for arduino-esp32 depends on the PC operating system:

- Windows: `C:\Users\<user name>\AppData\Local\Arduino15\packages\esp32`
- Linux: `~/.arduino15/packages/esp32`

The SDK for arduino-esp32 v3.x.x version is located in the `<arduino-esp32 root>/tools/esp32-arduino-libs/idf-release_x` directory within the default installation path.
