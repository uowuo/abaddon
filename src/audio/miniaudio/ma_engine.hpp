#pragma once

#include <memory>
#include <optional>
#include <string_view>

#include <miniaudio.h>

namespace AbaddonClient::Audio::Miniaudio {

class MaEngine {
public:
    static std::optional<MaEngine> Create(ma_engine_config &&config) noexcept;

    bool Start() noexcept;
    bool Stop() noexcept;

    bool PlaySound(std::string_view file_path) noexcept;

    ma_engine& GetInternal() noexcept;

private:
    struct EngineDeleter {
        void operator()(ma_engine* ptr) noexcept {
            ma_engine_uninit(ptr);
        }
    };

    // Put ma_engine behind pointer to allow moving.
    // miniaudio expects ma_engine reference to be valid at all times
    // Moving it to other location would cause memory corruption
    using EnginePtr = std::unique_ptr<ma_engine, EngineDeleter>;
    MaEngine(EnginePtr &&engine) noexcept;

    EnginePtr m_engine;
};

}
