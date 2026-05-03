/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include "chip/esp_expander_base.hpp"
#include "esp_panel_io_expander_conf_internal.h"

namespace esp_panel::drivers {

/**
 * @brief Base class for IO expander devices
 */
class IO_Expander {
public:
    /**
     * @brief Basic attributes structure for IO expander device
     */
    struct BasicAttributes {
        const char *name = "";  /*!< The chip name, defaults to `""` */
    };

    /**
     * @brief Configuration type alias
     */
    using Config = esp_expander::Base::Config;
    using HostPartialConfig = esp_expander::Base::HostPartialConfig;

    /**
     * @brief Construct a new IO expander device
     *
     * @param[in] attr Basic attributes for the IO expander device
     * @param[in] config Configuration for the IO expander device
     */
    IO_Expander(const BasicAttributes &attr, const esp_expander::Base::Config &config):
        _attritues(attr)
    {
        _is_skip_init_host = !config.isHostConfigValid();
    }

    /**
     * @brief Virtual destructor
     */
    virtual ~IO_Expander() = default;

    /**
     * @brief Initialize the IO expander device
     *
     * @return `true` if successful, `false` otherwise
     */
    virtual bool init() = 0;

    /**
     * @brief Start the IO expander device
     *
     * @return `true` if successful, `false` otherwise
     */
    virtual bool begin() = 0;

    /**
     * @brief Delete the IO expander device and release resources
     *
     * @return `true` if successful, `false` otherwise
     */
    virtual bool del() = 0;

    /**
     * @brief Check if device has reached specified state
     *
     * @param[in] state State to check against
     * @return `true` if current state >= given state, `false` otherwise
     */
    virtual bool isOverState(esp_expander::Base::State state) const = 0;

    /**
     * @brief Get base class pointer
     *
     * @return Pointer to base class
     */
    virtual esp_expander::Base *getBase() = 0;

    /**
     * @brief Skip the initialization of the host
     *
     * @return `true` if successful, `false` otherwise
     * @note This function should be called before `init()`
     * @note This function is useful when the host needs to be initialized outside the driver
     */
    bool skipInitHost()
    {
        _is_skip_init_host = true;
        return true;
    }

    /**
     * @brief Get basic attributes of the IO expander device
     *
     * @return Reference to basic attributes structure
     */
    const BasicAttributes &getBasicAttributes() const
    {
        return _attritues;
    }

protected:
    /**
     * @brief Check if host initialization should be skipped
     *
     * @return `true` if host initialization should be skipped, `false` otherwise
     */
    bool isSkipInitHost()
    {
        return _is_skip_init_host;
    }

private:
    bool _is_skip_init_host = false;         /*!< Flag to skip host initialization */
    BasicAttributes _attritues = {};        /*!< Basic device attributes */
};

} // namespace esp_panel::drivers
