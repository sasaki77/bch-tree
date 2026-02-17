#include <filesystem>
#include <future>

#include "epics/ca/ca_context_manager.h"

class SoftIocRunner {
   public:
    pid_t Start(const std::string& db_text);
    void KillIfRunning();
    void WriteDBtoTemp(const std::string& db_text);

   private:
    pid_t pid_{-1};
    std::filesystem::path temp_db_path_;
    std::future<int> waiter_;
};
