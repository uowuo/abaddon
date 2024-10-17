#pragma once

#include <memory>
#include <optional>

#include <miniaudio.h>

namespace AbaddonClient::Audio::Miniaudio {

class MaLog {
public:
    static std::optional<MaLog> Create() noexcept;

    bool RegisterCallback(ma_log_callback callback) noexcept;

    ma_log& GetInternal() noexcept;
private:
    struct LogDeleter {
        void operator()(ma_log* ptr) noexcept {
            ma_log_uninit(ptr);
        }
    };

    using LogPtr = std::unique_ptr<ma_log, LogDeleter>;
    MaLog(LogPtr &&log) noexcept;

    LogPtr m_log;
};

}
