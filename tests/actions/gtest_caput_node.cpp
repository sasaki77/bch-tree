#include <behaviortree_cpp/bt_factory.h>
#include <gtest/gtest.h>

#include <chrono>
#include <memory>
#include <sstream>
#include <string>
#include <thread>

#include "actions/caput_node.h"
#include "epics/ca/ca_pv.h"
#include "epics/ca/ca_pv_manager.h"
#include "epics/types.h"
#include "helper_func.h"
#include "node_test_helper.h"
#include "softioc_fixture.h"

using namespace bchtree;
using namespace bchtree::epics::ca;
using namespace std::chrono_literals;

template <typename T>
std::optional<T> ReadPV(
    std::shared_ptr<bchtree::epics::ca::CAContextManager> ctx,
    const std::string& pvname) {
    auto pv = CAPV(ctx, pvname);
    pv.Connect();

    if (!WaitUntilConnected(pv)) {
        return std::nullopt;
    }

    // Read back as requested type
    return pv.GetAs<T>();
}

// Helper to build/run a single CAPut node BehaviorTree
class CAPutNodeFactoryHelper {
   public:
    explicit CAPutNodeFactoryHelper(std::shared_ptr<CAContextManager> ctx)
        : ctx_(std::move(ctx)) {
        pv_manager_ = std::make_shared<PVManager>(ctx_);
        factory_ = std::make_shared<BT::BehaviorTreeFactory>();
        helper_ = std::make_unique<NodeTestHelper>(factory_);

        // Register 3 concrete node types
        factory_->registerNodeType<CAPutNode<double>>("CAPutDouble", ctx_,
                                                      pv_manager_);
        factory_->registerNodeType<CAPutNode<int>>("CAPutInt", ctx_,
                                                   pv_manager_);
        factory_->registerNodeType<CAPutNode<std::string>>("CAPutString", ctx_,
                                                           pv_manager_);
    }

    // Build a single-node tree from XML, run until it finishes, and return
    // status. Note: pass 'timeout' as a literal (no braces) to avoid Blackboard
    // substitution.
    BT::NodeStatus runSingle(
        const std::string& node_tag, const std::string& pv,
        const std::string& value_literal, int timeout_ms,
        bool force_write = false,
        std::chrono::milliseconds overall_timeout =
            std::chrono::milliseconds(3000),
        std::chrono::milliseconds step = std::chrono::milliseconds(20)) {
        // Compose XML for a single node tree
        std::ostringstream xml;
        xml << R"(<root BTCPP_format="4"><BehaviorTree ID="MainTree">)"
            << "<" << node_tag << " pv=\"" << pv << "\""
            << " value=\"" << value_literal << "\""
            << " timeout=\"" << timeout_ms << "\""
            << " force_write=\"" << (force_write ? "true" : "false") << "\""
            << "/>"
            << R"(</BehaviorTree></root>)";

        return helper_->runSingle(xml.str(), overall_timeout, step);
    }

    // Build a single-node tree from XML, run until it finishes, and return
    // status. Note: pass 'timeout' as a literal (no braces) to avoid Blackboard
    // substitution.
    BT::NodeStatus runOnce(const std::string& node_tag, const std::string& pv,
                           const std::string& value_literal, int timeout_ms,
                           bool force_write = false) {
        // Compose XML for a single node tree
        std::ostringstream xml;
        xml << R"(<root BTCPP_format="4"><BehaviorTree ID="MainTree">)"
            << "<" << node_tag << " pv=\"" << pv << "\""
            << " value=\"" << value_literal << "\""
            << " timeout=\"" << timeout_ms << "\""
            << " force_write=\"" << (force_write ? "true" : "false") << "\""
            << "/>"
            << R"(</BehaviorTree></root>)";

        return helper_->runOnce(xml.str());
    }

    std::shared_ptr<PVManager> GetPVManager() { return pv_manager_; }

   private:
    std::shared_ptr<CAContextManager> ctx_;
    std::shared_ptr<PVManager> pv_manager_;
    std::shared_ptr<BT::BehaviorTreeFactory> factory_;
    std::unique_ptr<NodeTestHelper> helper_;
};

TEST_F(SoftIocFixture, CAPutDouble_Succeeds_Then_ReadBack) {
    // Arrange
    const std::string pvname = "TEST:AO";
    const double target = 12.345;  // desired value
    CAPutNodeFactoryHelper helper(ctx_);

    auto status =
        helper.runSingle("CAPutDouble", pvname, std::to_string(target),
                         /*timeout_ms*/ 1500);
    EXPECT_EQ(status, BT::NodeStatus::SUCCESS);

    std::optional<double> got = ReadPV<double>(ctx_, pvname);
    ASSERT_TRUE(got.has_value());
    EXPECT_NEAR(*got, target, 1e-6);
}

TEST_F(SoftIocFixture, CAPutInt_Succeeds_Then_ReadBack) {
    const std::string pvname = "TEST:LO";
    const int target = 1234;
    CAPutNodeFactoryHelper helper(ctx_);

    auto status = helper.runSingle("CAPutInt", pvname, std::to_string(target),
                                   /*timeout_ms*/ 1500);
    EXPECT_EQ(status, BT::NodeStatus::SUCCESS);

    std::optional<int> got = ReadPV<int>(ctx_, pvname);
    ASSERT_TRUE(got.has_value());
    EXPECT_EQ(*got, target);
}

TEST_F(SoftIocFixture, CAPutString_Succeeds_Then_ReadBack) {
    const std::string pvname = "TEST:STRO";
    const std::string target = "hello-world";
    CAPutNodeFactoryHelper helper(ctx_);

    auto status =
        helper.runSingle("CAPutString", pvname, target, /*timeout_ms*/ 1500);
    EXPECT_EQ(status, BT::NodeStatus::SUCCESS);

    std::optional<std::string> got = ReadPV<std::string>(ctx_, pvname);
    ASSERT_TRUE(got.has_value());
    EXPECT_EQ(*got, target);
}

TEST_F(SoftIocFixture, CAPut_Timeouts_On_NonexistentPV) {
    // Use a PV that does not exist so it never connects.
    const std::string pv = "TEST:NONEXISTENT";
    CAPutNodeFactoryHelper helper(ctx_);
    auto status = helper.runSingle(
        "CAPutDouble", pv, "1.0", /*timeout_ms*/ 300, /*force_write*/ false,
        std::chrono::milliseconds(2000), std::chrono::milliseconds(10));
    // The node should return FAILURE upon timeout.
    EXPECT_EQ(status, BT::NodeStatus::FAILURE);
}

TEST_F(SoftIocFixture, CAPut_ShortCircuits_When_SameValue_And_ForceWriteFalse) {
    const std::string pv = "TEST:AO";
    const double target = 7.0;
    CAPutNodeFactoryHelper helper(ctx_);

    // Ensure that PV is already connected and keeps its pointer
    auto pv_manager = helper.GetPVManager();
    auto capv = pv_manager->Get(pv);
    capv->Connect();
    ASSERT_TRUE(WaitUntilConnected(*capv));

    // Pre-set value to target
    {
        auto status = helper.runSingle(
            "CAPutDouble", pv, std::to_string(target), /*timeout_ms*/ 1000);
        ASSERT_EQ(status, BT::NodeStatus::SUCCESS);
    }

    // Run again with the same value and force_write=false (default)
    // Expected: Since the PV is connected and current value == target, node
    // returns SUCCESS early.
    auto status = helper.runOnce("CAPutDouble", pv, std::to_string(target),
                                 /*timeout_ms*/ 1000, /*force_write*/ false);
    EXPECT_EQ(status, BT::NodeStatus::SUCCESS);

    const double got = capv->GetAs<double>();
    EXPECT_NEAR(got, target, 1e-6);
}

TEST_F(SoftIocFixture, CAPut_ForceWrite_True_Performs_Put_Even_If_SameValue) {
    const std::string pv = "TEST:LO";
    const int target = 99;
    CAPutNodeFactoryHelper helper(ctx_);

    // Ensure that PV is already connected and keeps its pointer
    auto pv_manager = helper.GetPVManager();
    auto capv = pv_manager->Get(pv);
    capv->Connect();
    ASSERT_TRUE(WaitUntilConnected(*capv));

    // Pre-set
    {
        auto status = helper.runSingle("CAPutInt", pv, std::to_string(target),
                                       /*timeout_ms*/ 1000);
        ASSERT_EQ(status, BT::NodeStatus::SUCCESS);
    }

    // Run with the same value but force_write=true
    // Expected: Since the PV is connected and current value != target, node
    // returns RUNNING.
    auto status = helper.runOnce("CAPutInt", pv, std::to_string(target),
                                 /*timeout_ms*/ 1000, /*force_write*/ true);
    EXPECT_EQ(status, BT::NodeStatus::RUNNING);

    const int got = capv->GetAs<int>();
    EXPECT_EQ(got, target);
}

TEST_F(SoftIocFixture, CAPut_Respects_CustomTimeout) {
    // Very small timeout to force a quick failure on a PV that won't exist.
    CAPutNodeFactoryHelper helper(ctx_);
    const std::string pv = "TEST:DOES_NOT_EXIST";
    auto status = helper.runSingle("CAPutString", pv, "x", /*timeout_ms*/ 50);
    EXPECT_EQ(status, BT::NodeStatus::FAILURE);
}
