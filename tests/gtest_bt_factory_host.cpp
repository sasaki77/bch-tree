#include <behaviortree_cpp/blackboard.h>
#include <gtest/gtest.h>

#include <chrono>
#include <future>
#include <thread>
#include <vector>

#include "bt_factory_host.h"

using namespace std::chrono_literals;

// Minimal XML that uses only the builtin Print node
static const char* kXmlPrintOnly = R"XML(
<root BTCPP_format="4">
  <BehaviorTree ID="PrintOnly">
    <Sequence name="root">
      <Print message="hello"/>
    </Sequence>
  </BehaviorTree>
</root>
)XML";

// Another tree ID to check registration of multiple trees
static const char* kXmlSecondTree = R"XML(
<root BTCPP_format="4">
  <BehaviorTree ID="Secondary">
    <Sequence name="root">
      <Print message="secondary"/>
    </Sequence>
  </BehaviorTree>
</root>
)XML";

// Helper to convert NodeStatus to string for readable ASSERT/EXPECT messages
static const char* ToStr(BT::NodeStatus s) {
    switch (s) {
        case BT::NodeStatus::IDLE:
            return "IDLE";
        case BT::NodeStatus::RUNNING:
            return "RUNNING";
        case BT::NodeStatus::SUCCESS:
            return "SUCCESS";
        case BT::NodeStatus::FAILURE:
            return "FAILURE";
        default:
            return "UNKNOWN";
    }
}

TEST(BTFactoryHost, RegisterTreeFromText_And_ListIDs) {
    bchtree::BTFactoryHost host;

    // Register builtin nodes (ctx/pv_manager not needed for Print-only tree)
    host.registerBuiltinNodes(/*ctx*/ nullptr, /*pv_manager*/ nullptr);

    // Register two tree definitions by text
    host.registerTreeFromText(kXmlPrintOnly);
    host.registerTreeFromText(kXmlSecondTree);

    // Verify registered IDs contain both names
    const auto ids = host.registeredTreeIDs();
    // We don't rely on order
    EXPECT_NE(std::find(ids.begin(), ids.end(), "PrintOnly"), ids.end());
    EXPECT_NE(std::find(ids.begin(), ids.end(), "Secondary"), ids.end());
}

TEST(BTFactoryHost, CreateTree_Without_ExplicitPrepareOnce) {
    bchtree::BTFactoryHost host;
    host.registerBuiltinNodes(nullptr, nullptr);
    host.registerTreeFromText(kXmlPrintOnly);

    // Do NOT call prepareOnce(); createTree should call ensurePrepared_
    // internally
    auto bb = BT::Blackboard::create();
    ASSERT_NO_THROW({
        auto tree = host.createTree("PrintOnly", bb);
        (void)tree;
    });
}

TEST(BTFactoryHost, PrepareOnce_Is_Idempotent) {
    bchtree::BTFactoryHost host;
    host.registerBuiltinNodes(nullptr, nullptr);
    host.registerTreeFromText(kXmlPrintOnly);

    // Call prepareOnce() multiple times. It should be safe/no-throw.
    ASSERT_NO_THROW(host.prepareOnce());
    ASSERT_NO_THROW(host.prepareOnce());
    ASSERT_NO_THROW(host.prepareOnce());
}

TEST(BTFactoryHost, Tick_PrintOnly_Tree_Returns_SUCCESS) {
    bchtree::BTFactoryHost host;
    host.registerBuiltinNodes(nullptr, nullptr);
    host.registerTreeFromText(kXmlPrintOnly);

    auto bb = BT::Blackboard::create();

    // Create the tree and tickWhileRunning with a small sleep
    auto tree = host.createTree("PrintOnly", bb);
    auto final_status = tree.tickWhileRunning(5ms);

    EXPECT_EQ(final_status, BT::NodeStatus::SUCCESS)
        << "Final status is " << ToStr(final_status);
}

TEST(BTFactoryHost, CreateTree_With_Unknown_ID_Throws) {
    bchtree::BTFactoryHost host;
    host.registerBuiltinNodes(nullptr, nullptr);
    host.registerTreeFromText(kXmlPrintOnly);

    auto bb = BT::Blackboard::create();

    // Unknown tree ID should throw BT::RuntimeError (BehaviorTreeFactory
    // behavior)
    EXPECT_THROW(
        {
            auto tree = host.createTree("NoSuchTree", bb);
            (void)tree;
        },
        std::runtime_error);
}

TEST(BTFactoryHost, CreateMultipleTrees_FromSameHost_Parallel) {
    bchtree::BTFactoryHost host;
    host.registerBuiltinNodes(nullptr, nullptr);

    // Register two different trees
    host.registerTreeFromText(kXmlPrintOnly);
    host.registerTreeFromText(kXmlSecondTree);

    // Prepare once before multi-threaded use
    host.prepareOnce();

    // Create multiple trees and tick them in parallel
    auto worker = [&host](const std::string id) {
        auto bb = BT::Blackboard::create();
        auto tree = host.createTree(id, bb);
        return tree.tickWhileRunning(2ms);
    };

    std::future<BT::NodeStatus> f1 =
        std::async(std::launch::async, worker, "PrintOnly");
    std::future<BT::NodeStatus> f2 =
        std::async(std::launch::async, worker, "Secondary");

    auto s1 = f1.get();
    auto s2 = f2.get();

    EXPECT_EQ(s1, BT::NodeStatus::SUCCESS) << "s1: " << ToStr(s1);
    EXPECT_EQ(s2, BT::NodeStatus::SUCCESS) << "s2: " << ToStr(s2);
}

// Optional: Plugin loading test. Disabled by default.
// Provide an environment variable BCHF_PLUGIN_PATH to run.
// The plugin must export BT_REGISTER_NODES(factory). See project docs.
TEST(BTFactoryHost, DISABLED_LoadPlugin_If_Available) {
    const char* path = std::getenv("BCHF_PLUGIN_PATH");
    if (!path) {
        GTEST_SKIP() << "Set BCHF_PLUGIN_PATH to a valid plugin (.so/.dll) to "
                        "enable this test.";
    }

    bchtree::BTFactoryHost host;
    host.addPluginPath(std::string(path));

    // No registrars; rely on plugin to register nodes/trees (if it does).
    // Here we only test that prepareOnce doesn't throw when loading the plugin.
    EXPECT_NO_THROW(host.prepareOnce());
}
