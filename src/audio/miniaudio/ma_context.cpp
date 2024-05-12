#include "ma_context.hpp"

namespace AbaddonClient::Audio::Miniaudio {

MaContext::MaContext(ContextPtr &&context) noexcept :
    m_context(std::move(context)) {}

std::optional<MaContext> MaContext::Create(ma_context_config &&config, ConstSlice<ma_backend> backends) noexcept {
    ContextPtr context = ContextPtr(new ma_context, &ma_context_uninit);

    const auto result = ma_context_init(backends.data(), backends.size(), &config, context.get());
    if (result != MA_SUCCESS) {
        spdlog::get("audio")->error("Failed to create context: {}", ma_result_description(result));
        return std::nullopt;
    }

    return std::make_optional<MaContext>(std::move(context));
}

std::optional<MaContext::DeviceInfo> MaContext::GetDevices() noexcept {
    ma_device_info* playback_device_infos;
    ma_uint32 playback_device_count;

    ma_device_info* capture_device_infos;
    ma_uint32 capture_device_count;

    const auto result = ma_context_get_devices(m_context.get(), &playback_device_infos, &playback_device_count, &capture_device_infos, &capture_device_count);
    if (result != MA_SUCCESS) {
        spdlog::get("audio")->error("Failed to get devices information: {}", ma_result_description(result));
        return std::nullopt;
    }

    const auto playback_info = PlaybackDeviceInfo(playback_device_infos, playback_device_count);
    const auto capture_info = CaptureDeviceInfo(capture_device_infos, capture_device_count);

    return std::make_optional<DeviceInfo>(std::move(playback_info), std::move(capture_info));
}

ma_context& MaContext::GetInternal() noexcept {
    return *m_context;
}

}
