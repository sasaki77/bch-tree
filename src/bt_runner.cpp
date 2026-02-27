#include "bt_runner.h"

#include <behaviortree_cpp/loggers/bt_cout_logger.h>
#include <behaviortree_cpp/xml_parsing.h>

#include "actions/caget_node.h"
#include "actions/camonitor_node.h"
#include "actions/caput_node.h"
#include "actions/print_node.h"
#include "bt_factory_host.h"

namespace bchtree {

void BTRunner::PrintTree(const std::string& tree_id) {
    auto tree = host_.createTree(tree_id);

    BT::printTreeRecursively(tree.rootNode());
}

bool BTRunner::Run(const std::string& tree_id, const BBInitMap& bb_map,
                   const std::chrono::milliseconds sleep_time) {
    auto bb = MakeBlackboard(bb_map);
    auto tree = host_.createTree(tree_id, bb);

    if (use_runner_logger_) {
        runner_logger_ = std::make_unique<RunnerLogger>(tree, logger_);
    }

    if (logger_) {
        logger_->info("Start Tree:");
    }

    const BT::NodeStatus status = tree.tickWhileRunning(sleep_time);

    if (logger_) {
        logger_->info(std::string("End Tree: status=") + toStr(status));
    }

    return status == BT::NodeStatus::SUCCESS;
}

void BTRunner::SetLogger(std::shared_ptr<Logger> logger) { logger_ = logger; }

void BTRunner::UseRunnerLogger() { use_runner_logger_ = true; }

BT::Blackboard::Ptr BTRunner::MakeBlackboard(
    const std::unordered_map<std::string, std::string>& bb_map) {
    auto bb = BT::Blackboard::create();
    for (const auto& [k, v] : bb_map) {
        bb->set(k, v);
    }
    return bb;
}

RunnerLogger::RunnerLogger(const BT::Tree& tree, std::shared_ptr<Logger> logger)
    : StatusChangeLogger(tree.rootNode()), logger_(std::move(logger)) {}
RunnerLogger::~RunnerLogger() = default;

void RunnerLogger::callback(BT::Duration timestamp, const BT::TreeNode& node,
                            BT::NodeStatus prev_status, BT::NodeStatus status) {
    // https://github.com/BehaviorTree/BehaviorTree.CPP/blob/master/src/loggers/bt_cout_logger.cpp
    using namespace std::chrono;

    constexpr const char* whitespaces = "                         ";
    constexpr size_t ws_count = 25;

    const std::string& name = node.name();
    const char* padding = &whitespaces[std::min(ws_count, name.size())];

    const std::string prev_str = BT::toStr(prev_status, /*colored=*/false);
    const std::string curr_str = BT::toStr(status, /*colored=*/false);

    char buffer[256];
    std::snprintf(buffer, sizeof(buffer), " %s%s %s -> %s", name.c_str(),
                  padding, prev_str.c_str(), curr_str.c_str());
    logger_->debug(buffer);
}

void RunnerLogger::flush() { logger_->flush(); }
}  // namespace bchtree
