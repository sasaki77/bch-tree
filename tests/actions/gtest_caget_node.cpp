#include <behaviortree_cpp/bt_factory.h>
#include <gtest/gtest.h>

#include <chrono>
#include <cstdlib>
#include <memory>
#include <sstream>
#include <string>
#include <thread>

#include "actions/caget_node.h"
#include "epics/ca/ca_pv.h"
#include "epics/ca/ca_pv_manager.h"
#include "epics/types.h"
#include "softioc_fixture.h"

using namespace bchtree;
using namespace bchtree::epics::ca;

class CAGetNodeFactoryHelper {
   public:
    CAGetNodeFactoryHelper(std::shared_ptr<CAContextManager> ctx)
        : ctx_(std::move(ctx)) {
        pv_manager_ = std::make_shared<PVManager>(ctx_);
        registerNode();
    }

    void registerNode() {
        factory_.registerNodeType<CAGetNode<double>>("CAGetDouble", ctx_,
                                                     pv_manager_);
        factory_.registerNodeType<CAGetNode<int>>("CAGetInt", ctx_,
                                                  pv_manager_);
        factory_.registerNodeType<CAGetNode<std::string>>("CAGetString", ctx_,
                                                          pv_manager_);
    }

    // Build a single-node tree from XML, run until it finishes, and return
    // status.
    BT::NodeStatus runSingle(
        const std::string& node_tag, const std::string& pv, int timeout_ms,
        bool use_monitor, const std::string& result_key,
        std::chrono::milliseconds overall_timeout =
            std::chrono::milliseconds(3000),
        std::chrono::milliseconds step = std::chrono::milliseconds(20)) {
        // Build XML: map output port "result" to {result_key}
        std::ostringstream xml;
        xml << R"(<root BTCPP_format="4"><BehaviorTree ID="MainTree">)"
            << "<" << node_tag << " pv=\"" << pv << "\""
            << " timeout=\"" << timeout_ms << "\""
            << " use_monitor=\"" << (use_monitor ? "true" : "false") << "\""
            << " result=\"{" << result_key << "}\"/>"
            << R"(</BehaviorTree></root>)";

        // Build the tree
        factory_.registerBehaviorTreeFromText(xml.str());
        blackboard_ = BT::Blackboard::create();
        tree_ = factory_.createTree("MainTree", blackboard_);

        // First tick (onStart)
        auto status = tree_.tickExactlyOnce();
        if (status == BT::NodeStatus::SUCCESS ||
            status == BT::NodeStatus::FAILURE) {
            return status;
        }

        // Continue ticking until completion or timeout
        const auto deadline =
            std::chrono::steady_clock::now() + overall_timeout;

        while (std::chrono::steady_clock::now() < deadline) {
            std::this_thread::sleep_for(step);
            status = tree_.tickExactlyOnce();
            if (status == BT::NodeStatus::SUCCESS ||
                status == BT::NodeStatus::FAILURE) {
                return status;
            }
        }
        return status;  // Caller decides if it stays RUNNING
    }

    template <typename T>
    bool getFromBB(const std::string& result_key, T& out) const {
        return blackboard_->get<T>(result_key, out);
    }

   private:
    std::shared_ptr<CAContextManager> ctx_;
    std::shared_ptr<PVManager> pv_manager_;
    BT::BehaviorTreeFactory factory_;
    BT::Tree tree_;
    std::shared_ptr<BT::Blackboard> blackboard_;
};

//  Get double from TEST:AO with use_monitor=false
TEST_F(SoftIocFixture, CAGetNode_GetDouble_FactoryHelper) {
    ASSERT_EQ(system("caput -t TEST:AO 12.3"), 0);
    CAGetNodeFactoryHelper helper(ctx_);

    const std::string key = "out";
    auto status = helper.runSingle("CAGetDouble", "TEST:AO",
                                   /*timeout_ms*/ 2000,
                                   /*use_monitor*/ false, key,
                                   std::chrono::milliseconds(3000));
    ASSERT_EQ(status, BT::NodeStatus::SUCCESS);

    double got{};
    ASSERT_TRUE(helper.getFromBB<double>(key, got));
    EXPECT_NEAR(got, 12.3, 1e-6);
}

// Get int from TEST:LO with use_monitor=false
TEST_F(SoftIocFixture, CAGetNode_GetLong_FactoryHelper) {
    ASSERT_EQ(system("caput -t TEST:LO 42"), 0);
    CAGetNodeFactoryHelper helper(ctx_);

    const std::string key = "out";
    auto status = helper.runSingle("CAGetInt", "TEST:LO", 2000, false, key);
    ASSERT_EQ(status, BT::NodeStatus::SUCCESS);

    long got{};
    ASSERT_TRUE(helper.getFromBB<long>(key, got));
    EXPECT_EQ(got, 42);
}

// Get std::string from TEST:STRO with use_monitor=false
TEST_F(SoftIocFixture, CAGetNode_GetString_FactoryHelper) {
    ASSERT_EQ(system("caput -t TEST:STRO Hello"), 0);
    CAGetNodeFactoryHelper helper(ctx_);

    const std::string key = "out";
    auto status =
        helper.runSingle("CAGetString", "TEST:STRO", 2000, false, key);
    ASSERT_EQ(status, BT::NodeStatus::SUCCESS);

    std::string got;
    ASSERT_TRUE(helper.getFromBB<std::string>(key, got));
    EXPECT_EQ(got, "Hello");
}

// use_monitor=true path (TEST:AO)
TEST_F(SoftIocFixture, CAGetNode_UseMonitor_FactoryHelper) {
    ASSERT_EQ(system("caput -t TEST:AO 7.5"), 0);
    CAGetNodeFactoryHelper helper(ctx_);

    const std::string key = "out";
    auto status = helper.runSingle("CAGetDouble", "TEST:AO", 2000, true, key,
                                   std::chrono::milliseconds(2000));
    ASSERT_EQ(status, BT::NodeStatus::SUCCESS);

    double got{};
    ASSERT_TRUE(helper.getFromBB<double>(key, got));
    EXPECT_NEAR(got, 7.5, 1e-6);
}

// Non-existent PV → timeout leads to FAILURE
TEST_F(SoftIocFixture, CAGetNode_Timeout_FactoryHelper) {
    CAGetNodeFactoryHelper helper(ctx_);

    const std::string key = "out";
    auto status = helper.runSingle("CAGetDouble", "TEST:DOES_NOT_EXIST",
                                   /*timeout_ms*/ 300,
                                   /*use_monitor*/ false, key,
                                   std::chrono::milliseconds(2000));
    ASSERT_EQ(status, BT::NodeStatus::FAILURE);
}
