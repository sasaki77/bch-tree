#include "softioc_fixture.h"

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <chrono>
#include <fstream>
#include <string>
#include <thread>

pid_t SoftIocFixture::Spawn(const std::string& cmd) {
    // Spawn a child process to run 'cmd' and return PID
    pid_t pid = fork();

    if (pid == 0) {
        execl("/bin/sh", "sh", "-c", cmd.c_str(), (char*)nullptr);

        // exec failed: exit child immediately (127 = command-not-found).
        // Use _exit() to avoid stdio/atexit effects.
        _exit(127);
    }
    return pid;
}

void SoftIocFixture::KillIfRunning(pid_t pid) {
    // If process is alive, try SIGTERM then SIGKILL
    if (pid <= 0) {
        return;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // Try graceful SIGINT
    kill(pid, SIGINT);
    for (int i = 0; i < 100; ++i) {  // 5s
        if (waitpid(pid, nullptr, WNOHANG) == pid) return;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // Then SIGTERM with longer grace
    kill(pid, SIGTERM);
    for (int i = 0; i < 100; ++i) {  // 5s
        if (waitpid(pid, nullptr, WNOHANG) == pid) return;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // Last resort
    kill(pid, SIGKILL);
    waitpid(pid, nullptr, 0);
}

// Write DB text to a safely created temp file under testing::TempDir()
std::filesystem::path SoftIocFixture::WriteDBtoTemp(
    const std::string& db_text) {
    const std::string base = ::testing::TempDir();

    if (base.empty()) {
        throw std::runtime_error("testing::TempDir() returned empty");
    }

    // Build mkstemps() template
    std::filesystem::path dir(base);
    std::string tmpl = (dir / "bch-db-XXXXXX.db").string();

    std::vector<char> buf(tmpl.begin(), tmpl.end());
    buf.push_back('\0');

    int fd = mkstemps(buf.data(), /*suffixlen=*/3);
    if (fd == -1) {
        throw std::runtime_error("mkstemps failed for: " + tmpl);
    }

    // Restrict permission
    fchmod(fd, 0600);

    // Close the fd immediately
    // subsequent writes will go through ofstream
    close(fd);

    // Open std::ofstream and enable exceptions.
    std::filesystem::path file_path(buf.data());
    std::ofstream ofs(file_path, std::ios::binary);
    ofs.exceptions(std::ios::badbit | std::ios::failbit);

    // Stream the text as-is
    ofs << db_text;

    // Ensure data is on disk; with exceptions set, errors throw here as
    // well.
    ofs.flush();

    return file_path;
}

void SoftIocFixture::SetUpTestSuite() {
    setenv("EPICS_CA_AUTO_ADDR_LIST", "NO", 1);
    setenv("EPICS_CA_ADDR_LIST", "127.0.0.1", 1);
    // setenv("EPICS_CA_MAX_ARRAY_BYTES", "10485760", 1); // if needed

    ctx_->EnsureAttached();

    const std::string db_text = R"DB(
            record(ao, "$(P)AO") {
                field(VAL,  "0")
                field(PINI, "YES")
            }
        )DB";

    temp_db_path_ = WriteDBtoTemp(db_text);

    std::string cmd =
        "softIoc -m \"P=TEST:\" -d \"" + temp_db_path_.string() + "\"";
    pid_ = Spawn(cmd);

    std::this_thread::sleep_for(std::chrono::milliseconds(800));
}

void SoftIocFixture::TearDownTestSuite() {
    KillIfRunning(pid_);

    // Remove DB file
    std::error_code ec;
    if (!temp_db_path_.empty()) {
        std::filesystem::remove(temp_db_path_, ec);
    }
}

TEST_F(SoftIocFixture, SoftIocPutThenGet) {
    int rc1 = system("caput -t TEST:AO 12.3");
    ASSERT_EQ(rc1, 0);

    FILE* fp = popen("caget -t TEST:AO", "r");
    ASSERT_NE(fp, nullptr);
    char buf[256]{};
    ASSERT_TRUE(fgets(buf, sizeof(buf), fp) != nullptr);
    pclose(fp);

    std::string got = buf;
    EXPECT_TRUE(got.rfind("12.3", 0) == 0);
}
