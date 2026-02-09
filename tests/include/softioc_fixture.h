#include <gtest/gtest.h>

#include <filesystem>

class SoftIocFixture : public ::testing::Test {
   protected:
    static inline pid_t pid_ = -1;
    static inline std::filesystem::path temp_db_path_;

    static void SetUpTestSuite();
    static void TearDownTestSuite();

    static pid_t Spawn(const std::string& cmd);
    static void KillIfRunning(pid_t pid);
    static std::filesystem::path WriteDBtoTemp(const std::string& db_text);
};
