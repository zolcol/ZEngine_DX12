#pragma once
#include <memory>

// Bắt buộc define trước khi include spdlog để lấy được File & Line
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#include <spdlog/spdlog.h>

class Log
{
public:
	static void Init();
	inline static std::shared_ptr<spdlog::logger>& GetEngineLogger() { return s_EngineLogger; }

private:
	static std::shared_ptr<spdlog::logger> s_EngineLogger;
};

// =====================================================================
// MACRO WRAPPER - DÙNG CÁC MACRO NÀY Ở KHẮP ENGINE
// =====================================================================

#ifdef _DEBUG
#define ENGINE_TRACE(...)    SPDLOG_LOGGER_TRACE(Log::GetEngineLogger(), __VA_ARGS__)
#define ENGINE_INFO(...)     SPDLOG_LOGGER_INFO(Log::GetEngineLogger(), __VA_ARGS__)
#define ENGINE_WARN(...)     SPDLOG_LOGGER_WARN(Log::GetEngineLogger(), __VA_ARGS__)
#define ENGINE_ERROR(...)    SPDLOG_LOGGER_ERROR(Log::GetEngineLogger(), __VA_ARGS__)
#define ENGINE_FATAL(...)    SPDLOG_LOGGER_CRITICAL(Log::GetEngineLogger(), __VA_ARGS__)
#else
	// Bản Client bỏ qua Trace/Info/Warn để tối ưu hiệu năng
#define ENGINE_TRACE(...)    (void)0
#define ENGINE_INFO(...)     (void)0
#define ENGINE_WARN(...)     (void)0
#define ENGINE_ERROR(...)    SPDLOG_LOGGER_ERROR(Log::GetEngineLogger(), __VA_ARGS__)
#define ENGINE_FATAL(...)    SPDLOG_LOGGER_CRITICAL(Log::GetEngineLogger(), __VA_ARGS__)
#endif