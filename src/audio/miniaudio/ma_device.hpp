#pragma once

#include <memory>
#include <optional>

#include <miniaudio.h>

#include "ma_context.hpp"

namespace AbaddonClient::Audio::Miniaudio {

class MaDevice {
public:
    static std::optional<MaDevice> Create(MaContext &context, ma_device_config &config) noexcept;

    bool Start() noexcept;
    bool Stop() noexcept;

    ma_device& GetInternal() noexcept;

private:
    struct DeviceDeleter {
        void operator()(ma_device* ptr) noexcept {
            ma_device_uninit(ptr);
        }
    };

    // Put ma_device behind pointer to allow moving
    // miniaudio expects ma_device reference to be valid at all times.
    // Moving it to other location would cause memory corruption
    using DevicePtr = std::unique_ptr<ma_device, DeviceDeleter>;
    MaDevice(DevicePtr &&device) noexcept;

    DevicePtr m_device;
};

}
