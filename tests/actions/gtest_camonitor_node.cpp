#include <behaviortree_cpp/bt_factory.h>
#include <gtest/gtest.h>

#include <chrono>
#include <cstdlib>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "actions/camonitor_node.h"
#include "epics/ca/ca_context_manager.h"
#include "epics/ca/ca_pv_manager.h"
#include "epics/types.h"
#include "node_test_helper.h"
#include "softioc_fixture.h"

using namespace std::chrono_literals;
using bchtree::CAMonitorNode;
namespace ca = bchtree::epics::ca;

// ----------helpers ----------

// Build CAMonitor node XML snippet
static std::string BuildCAMonitorXml(const std::string& node_tag,
                                     const std::string& pv, bool initial_fire,
                                     int queue_capacity,
                                     const std::string& result_key) {
    std::ostringstream xml;
    xml << R"(<root BTCPP_format="4"><BehaviorTree ID="MainTree">)"
        << "<" << node_tag << " pv=\"" << pv << "\""
        << " initial_fire=\"" << (initial_fire ? "true" : "false") << "\""
        << " queue_capacity=\"" << queue_capacity << "\""
        << " result=\"{" << result_key << "}\"/>"
        << R"(</BehaviorTree></root>)";
    return xml.str();
}

class CAMonitorNodeRunner {
   public:
    CAMonitorNodeRunner(const std::shared_ptr<ca::CAContextManager>& ctx)
        : ctx_(ctx) {
        pv_manager_ = std::make_shared<ca::PVManager>(ctx_);
        factory_ = std::make_shared<BT::BehaviorTreeFactory>();
        helper_ = std::make_unique<NodeTestHelper>(factory_);

        // Register the node types you need
        factory_->registerNodeType<CAMonitorNode<double>>("CAMonitorDouble",
                                                          ctx_, pv_manager_);
        factory_->registerNodeType<CAMonitorNode<int32_t>>("CAMonitorInt", ctx_,
                                                           pv_manager_);
        factory_->registerNodeType<CAMonitorNode<std::string>>(
            "CAMonitorString", ctx_, pv_manager_);
    }

    void buildTree(const std::string& node_tag, const std::string& pv,
                   bool initial_fire, int queue_capacity,
                   const std::string& result_key) {
        xml_ = BuildCAMonitorXml(node_tag, pv, initial_fire, queue_capacity,
                                 result_key);
        helper_->buildTree(xml_);
    }

    // Build a one-node tree and keep it
    BT::NodeStatus runSingle(
        std::chrono::milliseconds overall_timeout =
            std::chrono::milliseconds(3000),
        std::chrono::milliseconds step = std::chrono::milliseconds(20)) {
        BT::NodeStatus status = helper_->runSingle(overall_timeout, step);

        return status;  // Caller decides if it stays RUNNING
    }

    template <typename T>
    bool getFromBB(const std::string& result_key, T& out) const {
        return helper_->getFromBB(result_key, out);
    }

   private:
    std::shared_ptr<ca::CAContextManager> ctx_;
    std::shared_ptr<ca::PVManager> pv_manager_;
    std::shared_ptr<BT::BehaviorTreeFactory> factory_;
    std::string xml_;
    std::unique_ptr<NodeTestHelper> helper_;
};

// ===================================================================
// Tests
// ===================================================================

//
// initial_fire = true
//  - First tick returns SUCCESS with current value
//  - Next caput produces a new SUCCESS with the updated value
//
TEST_F(SoftIocFixture, CAMonitorNode_InitialFire_ThenNextCaput) {
    ASSERT_EQ(system("caput -t TEST:AO 12.3"), 0);
    CAMonitorNodeRunner runner(ctx_);

    const std::string key = "out";

    runner.buildTree("CAMonitorDouble", "TEST:AO",
                     /*initial_fire*/ true,
                     /*queue_capacity*/ 60, key);

    // First SUCCESS comes from initial_fire (GetAs)
    auto status = runner.runSingle();
    ASSERT_EQ(status, BT::NodeStatus::SUCCESS);
    double v = 0.0;
    ASSERT_TRUE(runner.getFromBB<double>(key, v));
    EXPECT_NEAR(v, 12.3, 1e-6);

    // No caput: expect RUNNING if we wait briefly (no queued sample)
    status = runner.runSingle(1s);
    EXPECT_EQ(status, BT::NodeStatus::RUNNING);  // timeout (no new sample)

    // Now produce an update and expect SUCCESS with the new value
    ASSERT_EQ(system("caput -t TEST:AO 23.4"), 0);
    status = runner.runSingle(2s);
    ASSERT_EQ(status, BT::NodeStatus::SUCCESS);

    double v1 = 0.0;
    ASSERT_TRUE(runner.getFromBB<double>(key, v1));
    EXPECT_NEAR(v1, 23.4, 1e-6);
}

//
// initial_fire = false
//  - First connect-time monitor must be dropped
//  - Without caput, we should remain RUNNING
//  - After caput, one SUCCESS with that value
//
TEST_F(SoftIocFixture, CAMonitorNode_NoInitialFire_FirstMonitorDropped) {
    // Ensure a defined starting value
    ASSERT_EQ(system("caput -t TEST:AO 7.89"), 0);
    CAMonitorNodeRunner runner(ctx_);

    const std::string key = "out";
    runner.buildTree("CAMonitorDouble", "TEST:AO",
                     /*initial_fire*/ false,
                     /*queue_capacity*/ 60, key);

    // No caput: should time out in RUNNING
    auto status = runner.runSingle(500ms);
    EXPECT_EQ(status, BT::NodeStatus::RUNNING);

    // First real update: expect one SUCCESS
    ASSERT_EQ(system("caput -t TEST:AO 8.01"), 0);
    status = runner.runSingle(2s);
    ASSERT_EQ(status, BT::NodeStatus::SUCCESS);

    double v = 0.0;
    ASSERT_TRUE(runner.getFromBB<double>(key, v));
    EXPECT_NEAR(v, 8.01, 1e-6);
}

//
// Burst updates are queued and delivered FIFO (within queue capacity)
//
TEST_F(SoftIocFixture, CAMonitorNode_BurstIsQueuedFIFO) {
    CAMonitorNodeRunner runner(ctx_);

    const std::string key = "out";
    runner.buildTree("CAMonitorDouble", "TEST:AO",
                     /*initial_fire*/ false,
                     /*queue_capacity*/ 60, key);

    // Start tree and return RUNNING
    auto status = runner.runSingle(1s);
    ASSERT_EQ(status, BT::NodeStatus::RUNNING);

    // Trigger a small burst
    ASSERT_EQ(system("caput -t TEST:AO 1.1"), 0);
    ASSERT_EQ(system("caput -t TEST:AO 2.2"), 0);
    ASSERT_EQ(system("caput -t TEST:AO 3.3"), 0);

    // Consume three SUCCESS in order
    std::vector<double> got;
    for (int i = 0; i < 3; ++i) {
        auto status = runner.runSingle(2s);
        ASSERT_EQ(status, BT::NodeStatus::SUCCESS);
        double v = 0.0;
        ASSERT_TRUE(runner.getFromBB<double>(key, v));
        got.push_back(v);
    }
    ASSERT_EQ(got.size(), 3u);
    EXPECT_NEAR(got[0], 1.1, 1e-6);
    EXPECT_NEAR(got[1], 2.2, 1e-6);
    EXPECT_NEAR(got[2], 3.3, 1e-6);
}

//
// String PV: stringout
//
TEST_F(SoftIocFixture, CAMonitorNode_Stringout) {
    CAMonitorNodeRunner runner(ctx_);

    const std::string key = "out";
    runner.buildTree("CAMonitorString", "TEST:STRO",
                     /*initial_fire*/ false,
                     /*queue_capacity*/ 60, key);

    // Start tree and return RUNNING
    auto status = runner.runSingle(1s);
    ASSERT_EQ(status, BT::NodeStatus::RUNNING);

    ASSERT_EQ(system("caput -t TEST:STRO hello"), 0);
    status = runner.runSingle(2s);
    ASSERT_EQ(status, BT::NodeStatus::SUCCESS);

    std::string s;
    ASSERT_TRUE(runner.getFromBB<std::string>(key, s));
    EXPECT_EQ(s, "hello");
}

//
// Int PV: longout
//
TEST_F(SoftIocFixture, CAMonitorNode_Longout) {
    CAMonitorNodeRunner runner(ctx_);

    const std::string key = "out";
    runner.buildTree("CAMonitorInt", "TEST:LO",
                     /*initial_fire*/ false,
                     /*queue_capacity*/ 60, key);

    // Start tree and return RUNNING
    auto status = runner.runSingle(1s);
    ASSERT_EQ(status, BT::NodeStatus::RUNNING);

    ASSERT_EQ(system("caput -t TEST:LO 10"), 0);
    status = runner.runSingle(2s);
    ASSERT_EQ(status, BT::NodeStatus::SUCCESS);

    int32_t n = 0;
    ASSERT_TRUE(runner.getFromBB<int32_t>(key, n));
    EXPECT_EQ(n, 10);
}
