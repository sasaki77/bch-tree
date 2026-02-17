#include "softioc_runner.h"

#include <gtest/gtest.h>
#include <sys/prctl.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <chrono>
#include <fstream>
using namespace std::chrono_literals;

pid_t SoftIocRunner::Start(const std::string& db_text) {
    WriteDBtoTemp(db_text);

    pid_ = fork();
    if (pid_ == -1) {
        throw std::runtime_error("fork failed");
    }

    if (pid_ == 0) {
        // --- Child ---
        // Ask kernel to deliver SIGTERM to us if parent dies.
        // This attribute survives execve.
        if (prctl(PR_SET_PDEATHSIG, SIGTERM) == -1) {
            _exit(127);  // do not flush stdio
        }
        // Close a small race: parent could have died just before prctl.
        if (getppid() == 1) {
            _exit(0);
        }

        execlp("softIoc", "softIoc", "-d", temp_db_path_.c_str(),
               (char*)nullptr);

        // If exec fails, exit immediately (127 is conventional for "command not
        // found").
        _exit(127);
    }

    // --- Parent ---
    // Launch a single reaper that will waitpid() exactly once.
    // This avoids polling loops and guarantees zombie reaping.
    waiter_ = std::async(std::launch::async, [pid = pid_]() -> int {
        int status = 0;
        pid_t r;
        do {
            r = ::waitpid(pid, &status, 0);  // blocking wait
        } while (r == -1 && errno == EINTR);
        return status;  // WEXITSTATUS/WTERMSIG can be inspected by caller
                        // if needed
    });

    return pid_;
}

void SoftIocRunner::KillIfRunning() {
    if (pid_ <= 0) {
        return;
    }

    // Try signals in increasing severity: SIGINT -> SIGTERM -> SIGKILL.
    auto wait_ready = [this](std::chrono::milliseconds dur) {
        if (waiter_.valid()) {
            return waiter_.wait_for(dur) == std::future_status::ready;
        }
        return false;
    };

    // Graceful: SIGINT and short wait
    ::kill(pid_, SIGINT);
    if (wait_ready(5s)) {
        // Child has exited; cleanup and return
        if (!temp_db_path_.empty()) {
            std::error_code ec;
            std::filesystem::remove(temp_db_path_, ec);
        }
        pid_ = -1;
        return;
    }

    // More forceful: SIGTERM and another wait
    ::kill(pid_, SIGTERM);
    if (wait_ready(5s)) {
        if (!temp_db_path_.empty()) {
            std::error_code ec;
            std::filesystem::remove(temp_db_path_, ec);
        }
        pid_ = -1;
        return;
    }

    // Last resort: SIGKILL and wait until the reaper finishes
    ::kill(pid_, SIGKILL);
    if (waiter_.valid()) {
        waiter_.wait();  // ensure the zombie is reaped
    }

    if (!temp_db_path_.empty()) {
        std::error_code ec;
        std::filesystem::remove(temp_db_path_, ec);
    }
    pid_ = -1;
}

// Write DB text to a safely created temp file under testing::TempDir()
void SoftIocRunner::WriteDBtoTemp(const std::string& db_text) {
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

    temp_db_path_ = file_path;
}
