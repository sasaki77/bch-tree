#pragma once
#include <string>

namespace bchtree {
class Logger {
   public:
    void setLevel(const std::string& level);
    void setFile(const std::string& path);
    void info(const std::string& msg);
    void warn(const std::string& msg);
    void error(const std::string& msg);

   private:
    void ensure_init();
    bool initialized_{false};
    std::string file_path_;
};
}  // namespace bchtree
