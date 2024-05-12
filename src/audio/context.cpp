#include "context.hpp"

namespace AbaddonClient::Audio {

Context::Context(Miniaudio::MaContext &&context) noexcept :
    m_context(std::move(context))
{
    PopulateDevices();
    FindDefaultDevices();
}

std::optional<Context> Context::Create(ma_context_config &&config, ConstSlice<ma_backend> backends) noexcept {
    auto context = Miniaudio::MaContext::Create(std::move(config), backends);
    if (!context) {
        return std::nullopt;
    }

    return std::make_optional<Context>(std::move(*context));
}


ConstSlice<ma_device_info> Context::GetPlaybackDevices() noexcept {
    return m_playback_devices;
}

ConstSlice<ma_device_info> Context::GetCaptureDevices() noexcept {
    return m_capture_devices;
}

std::optional<ma_device_id> Context::GetActivePlaybackID() noexcept {
    return m_active_playback_id;
}

std::optional<ma_device_id> Context::GetActiveCaptureID() noexcept {
    return m_active_capture_id;
}

Miniaudio::MaContext& Context::GetRaw() noexcept {
    return *m_context;
}

void Context::PopulateDevices() noexcept {
    auto result = m_context->GetDevices();
    if (!result) {
        return;
    }

    auto& playback_devices = result->first;
    auto& capture_devices = result->second;

    m_playback_devices.reserve(playback_devices.size());
    m_capture_devices.reserve(capture_devices.size());

    m_playback_devices.assign(playback_devices.begin(), playback_devices.end());
    m_capture_devices.assign(capture_devices.begin(), capture_devices.end());
}

void Context::FindDefaultDevices() noexcept {
    for (auto& playback : m_playback_devices) {
        if (playback.isDefault) {
            m_active_playback_id = playback.id;
        }
    }

    for (auto& capture : m_capture_devices) {
        if (capture.isDefault) {
            m_active_capture_id = capture.id;
        }
    }

    if (!m_active_playback_id) {
        spdlog::get("audio")->warn("No default playback device found");
    }

    if (!m_active_capture_id) {
        spdlog::get("audio")->warn("No default capture device found");
    }
}

}
