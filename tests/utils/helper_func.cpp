#include "helper_func.h"

#include <thread>

using bchtree::epics::ca::CAPV;
using namespace std::chrono_literals;

// Wait until CAPV::IsConnected() becomes true within timeout.
bool WaitUntilConnected(CAPV& pv, std::chrono::milliseconds timeout) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        if (pv.IsConnected()) return true;
        std::this_thread::sleep_for(50ms);
    }
    return false;
}
