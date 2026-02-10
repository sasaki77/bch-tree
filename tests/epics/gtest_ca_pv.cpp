#include <cadef.h>
#include <db_access.h>
#include <gtest/gtest.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <future>
#include <string>
#include <thread>
#include <vector>

#include "epics/ca/ca_pv.h"
#include "softioc_fixture.h"

using namespace std::chrono_literals;
using bchtree::epics::ca::CAPV;

// ---------- Helper functions ----------
// Wait until CAPV::IsConnected() becomes true within timeout.
static bool WaitUntilConnected(CAPV& pv,
                               std::chrono::milliseconds timeout = 4s) {
    const auto deadline = std::chrono::steady_clock::now() + timeout;
    while (std::chrono::steady_clock::now() < deadline) {
        if (pv.IsConnected()) return true;
        std::this_thread::sleep_for(50ms);
    }
    return false;
}

// Run 'caget -t <pv>' and return its output string.
static std::string RunCagetTrimmed(const char* pvname) {
    std::string cmd = std::string("caget -t ") + pvname;
    FILE* fp = popen(cmd.c_str(), "r");
    if (!fp) return {};
    char buf[512]{};
    if (!fgets(buf, sizeof(buf), fp)) {
        pclose(fp);
        return {};
    }
    pclose(fp);
    std::string s = buf;
    if (!s.empty() && (s.back() == '\n' || s.back() == '\r')) s.pop_back();
    return s;
}

TEST_F(SoftIocFixture, CAPV_GetPVname) {
    CAPV pv(ctx_, "TEST:AO");
    EXPECT_EQ(pv.GetPVname(), std::string("TEST:AO"));
}

TEST_F(SoftIocFixture, CAPV_OnConnection) {
    CAPV pv(ctx_, "TEST:AO");

    bool connection = false;
    pv.AddConnCB([&](bool connected) { connection = connected; });

    pv.Connect();
    ASSERT_TRUE(WaitUntilConnected(pv))
        << "CA connection did not become ready in time";
    EXPECT_TRUE(connection);

    bool is_connected{pv.IsConnected()};
    EXPECT_TRUE(is_connected);
}

TEST_F(SoftIocFixture, CAPV_PutCB_Double) {
    CAPV pv(ctx_, "TEST:AO");
    pv.Connect();
    ASSERT_TRUE(WaitUntilConnected(pv));

    std::promise<bool> done;
    auto fut = done.get_future();

    bool enq = pv.PutCB(12.3, [&](bool success) { done.set_value(success); });
    ASSERT_TRUE(enq) << "ca_put_callback enqueue failed";

    ASSERT_EQ(fut.wait_for(4s), std::future_status::ready);
    EXPECT_TRUE(fut.get());

    double rd = pv.GetAs<double>();
    EXPECT_NEAR(rd, 12.3, 1e-3);
}

// ---------- Put and Get tests with PutCB and GetAs ----------
template <class T>
struct PutInput;

// double
template <>
struct PutInput<double> {
    static std::string pvname() { return "TEST:AO"; }
    static double value() { return 12.3; }
};
// float
template <>
struct PutInput<float> {
    static std::string pvname() { return "TEST:AO"; }
    static float value() { return 6.25f; }
};
// int32_t
template <>
struct PutInput<int32_t> {
    static std::string pvname() { return "TEST:LO"; }
    static int32_t value() { return 42; }
};
// uint16_t
template <>
struct PutInput<uint16_t> {
    static std::string pvname() { return "TEST:LO"; }
    static uint16_t value() { return static_cast<uint16_t>(7); }
};
// std::string
template <>
struct PutInput<std::string> {
    static std::string pvname() { return "TEST:STRO"; }
    static std::string value() { return std::string("Hello"); }
};

template <typename T>
class PutCB_Typed : public SoftIocFixture {};

using TestTypes =
    ::testing::Types<double, float, int32_t, uint16_t, std::string>;
TYPED_TEST_SUITE(PutCB_Typed, TestTypes);

TYPED_TEST(PutCB_Typed, CAPV_PutCB_and_GetAs_GetCBAs) {
    using T = TypeParam;
    std::string pvname = PutInput<T>::pvname();
    CAPV pv(this->ctx_, pvname);
    pv.Connect();
    ASSERT_TRUE(WaitUntilConnected(pv))
        << "CA connection did not become ready in time";

    std::promise<bool> done;
    auto fut = done.get_future();

    // ---- Put Value ----
    T value = PutInput<T>::value();
    bool enq = pv.PutCB(value, [&](bool success) { done.set_value(success); });
    ASSERT_TRUE(enq) << "ca_put_callback enqueue failed";

    ASSERT_EQ(fut.wait_for(4s), std::future_status::ready);
    EXPECT_TRUE(fut.get()) << "Put callback reported failure";

    // ---- Get Value with GetAs ----
    T got = pv.GetAs<T>();
    EXPECT_EQ(got, value);

    // ---- Get Value with GetCBAs ----
    T got_cb_value;
    std::promise<void> got_cb;

    pv.GetCBAs<T>(
        [&](T value) {
            got_cb_value = value;
            got_cb.set_value();
        },
        std::chrono::milliseconds(1000));

    ASSERT_EQ(got_cb.get_future().wait_for(4s), std::future_status::ready)
        << "No get callback event";

    EXPECT_EQ(got_cb_value, value);
}
