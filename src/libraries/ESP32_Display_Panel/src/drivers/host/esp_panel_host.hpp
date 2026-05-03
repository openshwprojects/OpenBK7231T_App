/*
 * SPDX-FileCopyrightText: 2024-2025 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <array>
#include <memory>
#include "utils/esp_panel_utils_log.h"
#include "utils/esp_panel_utils_cxx.hpp"

namespace esp_panel::drivers {

/**
 * @brief The base bus host template class implementing a variant of the Singleton pattern
 *
 * @tparam Derived Derived class type
 * @tparam Config Configuration type
 * @tparam N Maximum number of instances allowed
 */
template <class Derived, typename Config, int N>
class Host {
public:
    static_assert(N > 0, "Number of instances must be greater than 0");

    /**
     * @brief Host handle type definition
     */
    using HostHandle = void *;

    /**
     * @brief Driver state enumeration
     */
    enum class State : uint8_t {
        DEINIT = 0,     /*!< Driver is not initialized */
        BEGIN,          /*!< Driver is initialized and ready */
    };

    /**
     * @brief Get the number of instances
     *
     * @return Number of instances
     */
    static int getInstanceCount()
    {
        return _instances.size();
    }

    /**
     * @brief Get a instance of the host
     *
     * @param[in] id Host ID
     * @param[in] config Host configuration
     * @return Shared pointer to the derived class instance, nullptr if failed
     */
    static std::shared_ptr<Derived> getInstance(int id, const Config &config);

    /**
     * @brief Get a instance of the host
     *
     * @param[in] id Host ID
     * @return Shared pointer to the derived class instance, nullptr if failed
     */
    static std::shared_ptr<Derived> getInstance(int id);

    /**
     * @brief Try to release the instance
     *
     * @param[in] id Host ID
     * @return `true` if successful, `false` otherwise
     */
    static bool tryReleaseInstance(int id);

    /**
     * @brief Virtual destructor
     */
    virtual ~Host() = default;

    /**
     * @brief Startup the host
     *
     * @return `true` if successful, `false` otherwise
     */
    virtual bool begin() = 0;

    /**
     * @brief Get the ID of the host
     *
     * @return Host ID
     */
    int getID() const
    {
        return _id;
    }

    /**
     * @brief Get the handle of the host
     *
     * @return Host handle
     */
    HostHandle getHandle() const
    {
        return host_handle;
    }

    /**
     * @brief Check if driver has reached specified state
     *
     * @param[in] state State to check against
     * @return `true` if current state >= given state, `false` otherwise
     */
    bool isOverState(State state)
    {
        return (_state >= state);
    }

protected:
    /**
     * @brief Protected constructor for derived classes
     *
     * @param[in] id Host ID
     * @param[in] config Host configuration
     */
    Host(int id, const Config &config): config(config), _id(id) {}

    /**
     * @brief Set driver state
     *
     * @param[in] state New state to set
     */
    void setState(State state)
    {
        _state = state;
    }

    Config config = {};                    /*!< Host configuration */
    HostHandle host_handle = nullptr;      /*!< Host handle */

private:
    /**
     * @brief Calibrate configuration when host already exists
     *
     * @param[in] config New configuration
     * @return `true` if successful, `false` otherwise
     */
    virtual bool calibrateConfig(const Config &config) = 0;

    int _id = -1;                         /*!< Host ID */
    State _state = State::DEINIT;         /*!< Current driver state */
    inline static std::array<std::shared_ptr<Derived>, N> _instances;  /*!< Array of host instances */
};

template <class Derived, typename Config, int N>
bool Host<Derived, Config, N>::tryReleaseInstance(int id)
{
    ESP_UTILS_LOG_TRACE_ENTER();

    ESP_UTILS_LOGD("Param: id(%d)", id);
    ESP_UTILS_CHECK_FALSE_RETURN((size_t)id < _instances.size(), false, "Invalid ID");

    if ((_instances[id] != nullptr) && (_instances[id].use_count() == 1)) {
        _instances[id] = nullptr;
        ESP_UTILS_LOGD("Release host(%d)", id);
    }

    ESP_UTILS_LOG_TRACE_EXIT();

    return true;
}

template <class Derived, typename Config, int N>
std::shared_ptr<Derived> Host<Derived, Config, N>::getInstance(int id, const Config &config)
{
    ESP_UTILS_LOG_TRACE_ENTER();

    ESP_UTILS_LOGD("Param: id(%d), config(@%p)", id, &config);
    ESP_UTILS_CHECK_FALSE_RETURN((size_t)id < _instances.size(), nullptr, "Invalid host ID");

    if (_instances[id] == nullptr) {
        ESP_UTILS_CHECK_EXCEPTION_RETURN(
            (_instances[id] = utils::make_shared<Derived>(id, config)), nullptr, "Create instance failed"
        );
        ESP_UTILS_LOGD("No instance exist, create new one(@%p)", _instances[id].get());
    } else {
        ESP_UTILS_LOGD("Instance exist(@%p)", _instances[id].get());

        Config new_config = config;
        ESP_UTILS_CHECK_FALSE_RETURN(
            _instances[id]->calibrateConfig(new_config), nullptr,
            "Calibrate configuration failed, attempt to configure host with a incompatible configuration"
        );
    }

    ESP_UTILS_LOG_TRACE_EXIT();

    return _instances[id];
}

template <class Derived, typename Config, int N>
std::shared_ptr<Derived> Host<Derived, Config, N>::getInstance(int id)
{
    ESP_UTILS_LOG_TRACE_ENTER();

    ESP_UTILS_LOGD("Param: id(%d)", id);
    ESP_UTILS_CHECK_FALSE_RETURN((size_t)id < _instances.size(), nullptr, "Invalid host ID");

    ESP_UTILS_LOG_TRACE_EXIT();

    return _instances[id];
}

} // namespace esp_panel::drivers
