#pragma once

#include "misc/slice.hpp"

#include <miniaudio.h>

namespace AbaddonClient::Audio::Miniaudio {

class MaContext {
public:
    using PlaybackDeviceInfo = ConstSlice<ma_device_info>;
    using CaptureDeviceInfo = ConstSlice<ma_device_info>;
    using DeviceInfo = std::pair<PlaybackDeviceInfo, CaptureDeviceInfo>;

    static std::optional<MaContext> Create(ma_context_config &&config, ConstSlice<ma_backend> backends) noexcept;

    std::optional<DeviceInfo> GetDevices() noexcept;

    ma_context& GetInternal() noexcept;

private:
    // Put ma_context behind pointer to allow moving.
    // miniaudio expects ma_context reference to be valid at all times
    // Moving it to other location would cause memory corruption
    using ContextPtr = std::unique_ptr<ma_context, decltype(&ma_context_uninit)>;
    MaContext(ContextPtr &&context) noexcept;

    ContextPtr m_context;
};

}
