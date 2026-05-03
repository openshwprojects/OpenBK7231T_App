/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include "utils/esp_panel_utils_log.h"
#include "utils/esp_panel_utils_cxx.hpp"
#include "drivers/host/esp_panel_host_i2c.hpp"
#include "esp_panel_io_expander.hpp"

namespace esp_panel::drivers {


/**
 * @brief Adapter class template for IO expander devices
 *
 * @tparam T Type of the IO expander device, must satisfy CheckIsIO_ExpanderAdaptee concept
 */
template <class T>
class IO_ExpanderAdapter: public IO_Expander, public T {
public:
    static_assert(std::is_base_of_v<esp_expander::Base, T>, "`T` must be derived from `esp_expander::Base`");

    /**
     * @brief Construct a new IO expander adapter
     *
     * @param[in] attr Basic attributes for the IO expander device
     * @param[in] config Configuration for the IO expander device
     */
    IO_ExpanderAdapter(const BasicAttributes &attr, const esp_expander::Base::Config &config):
        IO_Expander(attr, config),
        T(config)
    {
    }

    /**
     * @brief Destroy the IO expander adapter
     */
    ~IO_ExpanderAdapter() override;

    /**
     * @brief Initialize the IO expander device
     *
     * @return `true` if successful, `false` otherwise
     */
    bool init() override;

    /**
     * @brief Start the IO expander device
     *
     * @return `true` if successful, `false` otherwise
     */
    bool begin() override;

    /**
     * @brief Delete the IO expander device and release resources
     *
     * @return `true` if successful, `false` otherwise
     */
    bool del() override;

    /**
     * @brief Check if device has reached specified state
     *
     * @param[in] state State to check against
     * @return `true` if current state >= given state, `false` otherwise
     */
    bool isOverState(esp_expander::Base::State state) const override
    {
        return T::isOverState(state);
    }

    /**
     * @brief Get base class pointer
     *
     * @return Pointer to base class
     */
    esp_expander::Base *getBase() override
    {
        return static_cast<esp_expander::Base *>(this);
    }

public:
    std::shared_ptr<HostI2C> _host = nullptr;  /*!< I2C host interface */
};

template <class T>
IO_ExpanderAdapter<T>::~IO_ExpanderAdapter()
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_EXIT(del(), "Delete failed");

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();
}

template <class T>
bool IO_ExpanderAdapter<T>::init()
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    // Skip host initialization first and do it later
    T::configHostSkipInit(true);

    ESP_UTILS_CHECK_FALSE_RETURN(T::init(), false, "Init base failed");

    // Since the host full configuration is converted from the partial configuration, we need to call `init` to
    // get the full configuration
    if (!this->IO_Expander::isSkipInitHost()) {
        ESP_UTILS_CHECK_FALSE_RETURN(
            std::holds_alternative<esp_expander::Base::HostFullConfig>(this->getConfig().host.value()),
            false, "Host config is not full"
        );

        auto host_config = std::get<esp_expander::Base::HostFullConfig>(this->getConfig().host.value());
        auto host_id = this->getConfig().host_id;
        _host = HostI2C::getInstance(host_id, host_config);
        ESP_UTILS_CHECK_NULL_RETURN(_host, false, "Get I2C host(%d) instance failed", host_id);

        ESP_UTILS_LOGD("Get I2C host(%d) instance", host_id);
    }

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

template <class T>
bool IO_ExpanderAdapter<T>::begin()
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    ESP_UTILS_CHECK_FALSE_RETURN(!this->isOverState(T::State::BEGIN), false, "Already begun");

    if (!this->isOverState(T::State::INIT)) {
        ESP_UTILS_CHECK_FALSE_RETURN(this->init(), false, "Init failed");
    }

    if (_host != nullptr) {
        int host_id = this->getConfig().host_id;
        ESP_UTILS_CHECK_FALSE_RETURN(_host->begin(), false, "Init host(%d) failed", host_id);
        ESP_UTILS_LOGD("Begin I2C host(%d)", host_id);
    }

    ESP_UTILS_CHECK_FALSE_RETURN(T::begin(), false, "Begin base failed");

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

template <class T>
bool IO_ExpanderAdapter<T>::del()
{
    ESP_UTILS_LOG_TRACE_ENTER_WITH_THIS();

    if (_host != nullptr) {
        _host = nullptr;
        int host_id = this->getConfig().host_id;
        ESP_UTILS_CHECK_FALSE_RETURN(
            HostI2C::tryReleaseInstance(host_id), false, "Release I2C host(%d) failed", host_id
        );
    }

    ESP_UTILS_CHECK_FALSE_RETURN(T::del(), false, "Delete base failed");

    ESP_UTILS_LOG_TRACE_EXIT_WITH_THIS();

    return true;
}

} // namespace esp_panel::drivers
