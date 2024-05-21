#include "ma_device.hpp"

namespace AbaddonClient::Audio::Miniaudio {

MaDevice::MaDevice(DevicePtr &&device) noexcept :
    m_device(std::move(device)) {}

std::optional<MaDevice> MaDevice::Create(MaContext &context, ma_device_config &config) noexcept {
    DevicePtr device = DevicePtr(new ma_device, &ma_device_uninit);

    const auto result = ma_device_init(&context.GetInternal(), &config, device.get());
    if (result != MA_SUCCESS) {
        spdlog::get("audio")->error("Failed to create MaDevice: {}", ma_result_description(result));
        return std::nullopt;
    }

    return std::move(device);
}

bool MaDevice::Start() noexcept {
    const auto result = ma_device_start(m_device.get());
    if (result != MA_SUCCESS) {
        spdlog::get("audio")->error("Failed to start device: {}", ma_result_description(result));
        return false;
    }

    return true;
}

bool MaDevice::Stop() noexcept {
    const auto result = ma_device_stop(m_device.get());
    if (result != MA_SUCCESS) {
        spdlog::get("audio")->error("Failed to stop device: {}", ma_result_description(result));
        return false;
    }

    return true;
}

ma_device& MaDevice::GetInternal() noexcept {
    return *m_device;
}

}
