#include "ma_log.hpp"

#include <utility>

#include <spdlog/spdlog.h>

namespace AbaddonClient::Audio::Miniaudio {

MaLog::MaLog(LogPtr &&log) noexcept :
    m_log(std::move(log)) {}

std::optional<MaLog> MaLog::Create() noexcept {
    LogPtr log = LogPtr(new ma_log);

    const auto result = ma_log_init(nullptr, log.get());
    if (result != MA_SUCCESS) {
        spdlog::get("audio")->error("Failed to create log: {}", ma_result_description(result));
        return std::nullopt;
    }

    return MaLog(std::move(log));
}

bool MaLog::RegisterCallback(ma_log_callback callback) noexcept {
    return ma_log_register_callback(m_log.get(), callback) == MA_SUCCESS;
}

ma_log& MaLog::GetInternal() noexcept {
    return *m_log;
}

}
