#include <behaviortree_cpp/bt_factory.h>
#include <gtest/gtest.h>

#include "actions/print_node.h"

namespace bchtree {

class PrintNodeFixture : public ::testing::Test {
   protected:
    BT::BehaviorTreeFactory factory;
    std::ostringstream oss;

    void SetUp() override {
        factory.registerNodeType<bchtree::PrintNode>("Print");
    }
};

TEST_F(PrintNodeFixture, ReturnsSuccessWithMessage) {
    const char* xml = R"(
<root BTCPP_format="4">
  <BehaviorTree ID="Main">
    <Print message="Hello, BT!" />
  </BehaviorTree>
</root>)";

    auto tree = factory.createTreeFromText(xml);
    auto status = tree.tickExactlyOnce();
    EXPECT_EQ(status, BT::NodeStatus::SUCCESS);
}

TEST_F(PrintNodeFixture, ThrowsWhenMessageMissing) {
    const char* xml = R"(
<root BTCPP_format="4">
  <BehaviorTree ID="Main">
    <Print />
  </BehaviorTree>
</root>)";

    auto tree = factory.createTreeFromText(xml);
    EXPECT_THROW({ (void)tree.tickExactlyOnce(); }, BT::RuntimeError);
}

}  // namespace bchtree
