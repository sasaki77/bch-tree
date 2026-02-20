#include "logger.h"

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#include <memory>
#include <vector>

namespace bchtree {

spdlog::level::level_enum Logger::to_level(const std::string& level) {
    if (level == "trace") return spdlog::level::trace;
    if (level == "debug") return spdlog::level::debug;
    if (level == "info") return spdlog::level::info;
    if (level == "warn") return spdlog::level::warn;
    if (level == "error") return spdlog::level::err;
    if (level == "critical") return spdlog::level::critical;
    if (level == "off") return spdlog::level::off;
    return spdlog::level::info;
}

void Logger::setConsoleLevel(const std::string& level) {
    console_level_ = to_level(level);
    if (initialized_) {
        rebuild_logger();
    }
}

void Logger::setFileLevel(const std::string& level) {
    file_level_ = to_level(level);
    if (initialized_) {
        rebuild_logger();
    }
}

void Logger::setFile(const std::string& path) {
    // Store path; if already initialized, rebuild to apply new file sink
    file_path_ = path;
    if (initialized_) {
        rebuild_logger();
    }
}

void Logger::ensure_init() {
    // Build logger only once; this does not touch the global default logger
    if (initialized_) return;
    rebuild_logger();
    initialized_ = true;
}

void Logger::rebuild_logger() {
    // Build sinks: console sink is always enabled
    auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    console_sink->set_level(console_level_);
    std::vector<spdlog::sink_ptr> sinks{console_sink};

    // If a file path is configured, add a file sink (append mode)
    if (!file_path_.empty()) {
        auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
            file_path_, /*truncate=*/false);
        file_sink->set_level(file_level_);
        sinks.push_back(file_sink);
    }

    // Create or replace the dedicated logger instance (do not change default
    // logger)
    logger_ =
        std::make_shared<spdlog::logger>("bt", sinks.begin(), sinks.end());

    // Apply pattern to this logger explicitly
    logger_->set_pattern("%Y-%m-%dT%H:%M:%S.%e %z [%l] %v");

    // Set logger level permissive; sinks filter effectively
    logger_->set_level(spdlog::level::trace);
    logger_->flush_on(spdlog::level::info);
}

void Logger::info(const std::string& msg) {
    ensure_init();
    logger_->info(msg);
}
void Logger::warn(const std::string& msg) {
    ensure_init();
    logger_->warn(msg);
}
void Logger::error(const std::string& msg) {
    ensure_init();
    logger_->error(msg);
}
void Logger::debug(const std::string& msg) {
    ensure_init();
    logger_->debug(msg);
}
void Logger::trace(const std::string& msg) {
    ensure_init();
    logger_->trace(msg);
}
void Logger::critical(const std::string& msg) {
    ensure_init();
    logger_->critical(msg);
}

void Logger::flush() {
    // Flush the dedicated logger only
    if (logger_) {
        logger_->flush();
    }
}

}  // namespace bchtree
