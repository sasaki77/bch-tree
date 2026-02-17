#include "node_test_helper.h"

NodeTestHelper::NodeTestHelper(std::shared_ptr<BT::BehaviorTreeFactory> factory)
    : factory_(std::move(factory)) {}

BT::NodeStatus NodeTestHelper::runSingle(
    std::string xml,
    std::chrono::milliseconds overall_timeout = std::chrono::milliseconds(3000),
    std::chrono::milliseconds step = std::chrono::milliseconds(20)) {
    // Build the tree
    factory_->registerBehaviorTreeFromText(xml);
    blackboard_ = BT::Blackboard::create();
    tree_ = factory_->createTree("MainTree", blackboard_);

    // First tick (onStart)
    auto status = tree_.tickExactlyOnce();
    if (status == BT::NodeStatus::SUCCESS ||
        status == BT::NodeStatus::FAILURE) {
        return status;
    }

    // Continue ticking until completion or timeout
    const auto deadline = std::chrono::steady_clock::now() + overall_timeout;

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

BT::NodeStatus NodeTestHelper::runOnce(std::string xml) {
    // Build the tree
    factory_->registerBehaviorTreeFromText(xml);
    blackboard_ = BT::Blackboard::create();
    tree_ = factory_->createTree("MainTree", blackboard_);

    auto status = tree_.tickExactlyOnce();
    return status;
}
