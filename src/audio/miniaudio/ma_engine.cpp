#include "ma_engine.hpp"

#include <utility>

#include <spdlog/spdlog.h>

namespace AbaddonClient::Audio::Miniaudio {

MaEngine::MaEngine(EnginePtr &&engine) noexcept :
    m_engine(std::move(engine)) {}

std::optional<MaEngine> MaEngine::Create(ma_engine_config &&config) noexcept {
    EnginePtr engine = EnginePtr(new ma_engine);

    const auto result = ma_engine_init(&config, engine.get());
    if (result != MA_SUCCESS) {
        spdlog::get("audio")->error("Failed to create engine: {}", ma_result_description(result));
        return std::nullopt;
    }

    return MaEngine(std::move(engine));
}

bool MaEngine::Start() noexcept {
    const auto result = ma_engine_start(m_engine.get());
    if (result != MA_SUCCESS) {
        spdlog::get("audio")->error("Failed to start engine: {}", ma_result_description(result));
        return false;
    }

    return true;
}

bool MaEngine::Stop() noexcept {
    const auto result = ma_engine_stop(m_engine.get());
    if (result != MA_SUCCESS) {
        spdlog::get("audio")->error("Failed to stop engine: {}", ma_result_description(result));
        return false;
    }

    return true;
}

bool MaEngine::PlaySound(std::string_view file_path) noexcept {
    const auto result = ma_engine_play_sound(m_engine.get(), file_path.data(), nullptr);
    if (result != MA_SUCCESS) {
        spdlog::get("audio")->error("Failed to play sound at {}: {}", file_path.data(), ma_result_description(result));
        return false;
    }

    return true;
}

ma_engine& MaEngine::GetInternal() noexcept {
    return *m_engine;
}

}
