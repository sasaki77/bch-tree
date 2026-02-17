#include <chrono>

#include "epics/ca/ca_pv.h"

bool WaitUntilConnected(
    bchtree::epics::ca::CAPV& pv,
    std::chrono::milliseconds timeout = std::chrono::milliseconds{4000});
