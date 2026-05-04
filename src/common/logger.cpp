#include "common/logger.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <vector>
#include <memory>
#include <iostream>

namespace interview
{
	namespace common
	{
		std::shared_ptr<spdlog::logger> Logger::logger_ = nullptr; // 初始化实例

		void Logger::Init(const std::string& log_file, bool debug_mode)
		{
			try
			{
				// 创建多个sink: 控制台+文件
				std::vector<spdlog::sink_ptr> sinks; // 存放多个输出目的地(信息输出的地方)

				// 1.彩色控制输出
				auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
				console_sink->set_level(debug_mode ? spdlog::level::debug : spdlog::level::info); // 动态调整日志等级
				console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
				sinks.push_back(console_sink); // 把配置好的目的地放入目的地清单中

				// 2.文件输出
				auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_file, true);
				file_sink->set_level(spdlog::level::trace); // 文件记录所有的级别
				file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [%s:%#] %v");
				sinks.push_back(file_sink);

				// 创建logger
				logger_ = std::make_shared<spdlog::logger>("interview", sinks.begin(), sinks.end());// 命名  控制台和文件输出
				logger_->set_level(debug_mode ? spdlog::level::debug : spdlog::level::info);
				logger_->flush_on(spdlog::level::warn); // warn以上级别立即刷新

				// 设置为默认 logger
				spdlog::set_default_logger(logger_);

				LOG_INFO("Logger initialized - Debug mode: {}", debug_mode);
			}
			catch (const spdlog::spdlog_ex& ex)
			{
				std::cerr << "Log initialization failed: " << ex.what() << std::endl;
			}
		}

		std::shared_ptr<spdlog::logger>& Logger::GetLogger()
		{
			if (!logger_)
			{
				Init(); // 如果未初始化使用默认配置
			}
			return logger_;
		}
	}
}