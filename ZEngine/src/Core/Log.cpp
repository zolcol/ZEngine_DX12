#include "Log.h"

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/msvc_sink.h>

std::shared_ptr<spdlog::logger> Log::s_EngineLogger;

void Log::Init()
{
    std::vector<spdlog::sink_ptr> logSinks;

    // 1. Kênh Console (Bật màu)
    logSinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());

    // 2. Kênh Visual Studio Output (Để đọc lỗi DX12)
    logSinks.emplace_back(std::make_shared<spdlog::sinks::msvc_sink_mt>());

    // Định dạng gọn gàng: [Thời gian] Tên_File:Dòng : Nội_dung
    for (auto& sink : logSinks)
    {
        sink->set_pattern("%^[%T] %s:%# : %v%$");
    }

    s_EngineLogger = std::make_shared<spdlog::logger>("ENGINE", begin(logSinks), end(logSinks));
    spdlog::register_logger(s_EngineLogger);

    // Cấu hình mức độ cảnh báo và tự động đẩy log ngay lập tức
    s_EngineLogger->set_level(spdlog::level::trace);
    s_EngineLogger->flush_on(spdlog::level::trace);
}