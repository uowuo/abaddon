#pragma once

#include "miniaudio/ma_context.hpp"

namespace AbaddonClient::Audio {

class Context {
public:
    static std::optional<Context> Create(ma_context_config &&config, ConstSlice<ma_backend> backends) noexcept;

    ConstSlice<ma_device_info> GetPlaybackDevices() noexcept;
    ConstSlice<ma_device_info> GetCaptureDevices() noexcept;

    std::optional<ma_device_id> GetActivePlaybackID() noexcept;
    std::optional<ma_device_id> GetActiveCaptureID() noexcept;

    Miniaudio::MaContext& GetRaw() noexcept;

private:
    Context(Miniaudio::MaContext &&context) noexcept;

    void PopulateDevices() noexcept;
    void FindDefaultDevices() noexcept;

    std::optional<ma_device_id> m_active_playback_id;
    std::optional<ma_device_id> m_active_capture_id;

    std::vector<ma_device_info> m_playback_devices;
    std::vector<ma_device_info> m_capture_devices;

    std::optional<Miniaudio::MaContext> m_context;
};


}
