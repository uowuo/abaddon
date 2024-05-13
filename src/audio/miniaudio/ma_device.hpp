#pragma once

#include <miniaudio.h>

#include "ma_context.hpp"

namespace AbaddonClient::Audio::Miniaudio {

class MaDevice {
public:
    // Put ma_device behind pointer to allow moving
    // miniaudio expects ma_device reference to be valid at all times.
    // Moving it to other location would cause memory corruption
    using DevicePtr = std::unique_ptr<ma_device, decltype(&ma_device_uninit)>;
    MaDevice(DevicePtr &&device) noexcept;

    static std::optional<MaDevice> Create(MaContext &context, ma_device_config &config) noexcept;

    bool Start() noexcept;
    bool Stop() noexcept;

    ma_device& GetInternal() noexcept;

private:
    DevicePtr m_device;
};

}
