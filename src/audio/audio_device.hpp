#pragma once

#include "context.hpp"

#include "miniaudio/ma_device.hpp"

namespace AbaddonClient::Audio {

class AudioDevice {
public:
    AudioDevice(Context& context, ma_device_config &&config, std::optional<ma_device_id> &&device_id) noexcept;

    bool Start() noexcept;
    bool Stop() noexcept;

    bool ChangeDevice(const ma_device_id &device_id) noexcept;
private:
    void SyncDeviceID() noexcept;
    bool RefreshDevice() noexcept;

    bool m_started = false;

    Context &m_context;
    std::optional<Miniaudio::MaDevice> m_device;

    ma_device_config m_config;
    std::optional<ma_device_id> m_device_id;
};

}
