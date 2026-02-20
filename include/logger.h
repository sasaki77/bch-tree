#pragma once
#include <spdlog/spdlog.h>

#include <string>

namespace bchtree {
class Logger {
   public:
    void setConsoleLevel(const std::string& level);
    void setFileLevel(const std::string& level);
    void setFile(const std::string& path);

    void info(const std::string& msg);
    void warn(const std::string& msg);
    void error(const std::string& msg);
    void debug(const std::string& msg);
    void trace(const std::string& msg);
    void critical(const std::string& msg);
    void off(const std::string& msg);
    void flush();

   private:
    void ensure_init();
    void rebuild_logger();
    bool initialized_{false};
    std::shared_ptr<spdlog::logger> logger_;
    std::string file_path_;

    spdlog::level::level_enum console_level_{spdlog::level::info};
    spdlog::level::level_enum file_level_{spdlog::level::debug};
    static spdlog::level::level_enum to_level(const std::string&);
};
}  // namespace bchtree
