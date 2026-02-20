#include "bt_runner.h"

#include <behaviortree_cpp/loggers/bt_cout_logger.h>
#include <behaviortree_cpp/xml_parsing.h>

#include "actions/caget_node.h"
#include "actions/caput_node.h"
#include "actions/print_node.h"

namespace bchtree {

void BTRunner::PrintTree() {
    if (!initialized_) {
        throw BT::RuntimeError("BTRunner: Runner is not initialized");
    }

    BT::printTreeRecursively(tree_.rootNode());
}

bool BTRunner::Run(std::chrono::milliseconds sleep_time) {
    if (!initialized_) {
        if (logger_) {
            logger_->info("BTRunner: Runner is not initialized");
        }
        return false;
    }

    if (logger_) {
        logger_->info("Start Tree:");
    }

    if (use_runner_logger_) {
        runner_logger_ = std::make_unique<RunnerLogger>(tree_, logger_);
    }

    const BT::NodeStatus status = tree_.tickWhileRunning(sleep_time);

    if (logger_) {
        logger_->info(std::string("End Tree: status=") + toStr(status));
    }

    return status == BT::NodeStatus::SUCCESS;
}

void BTRunner::SetLogger(std::shared_ptr<Logger> logger) { logger_ = logger; }

void BTRunner::SetGlobalBB(std::string key, std::string value) {
    globals_bb_map_[std::move(key)] = std::move(value);
}

void BTRunner::UseRunnerLogger() { use_runner_logger_ = true; }

void BTRunner::RegisterTreeFromFile(const std::string& treePath) {
    blackboard_ = BT::Blackboard::create();
    for (const auto& [k, v] : globals_bb_map_) {
        blackboard_->set(k, v);
    }

    factory_.registerNodeType<CAGetNode<epics::PVData>>("CAGet", ctx_,
                                                        pv_manager_);
    factory_.registerNodeType<CAGetNode<double>>("CAGetDouble", ctx_,
                                                 pv_manager_);
    factory_.registerNodeType<CAGetNode<int>>("CAGetInt", ctx_, pv_manager_);
    factory_.registerNodeType<CAGetNode<std::string>>("CAGetString", ctx_,
                                                      pv_manager_);

    factory_.registerNodeType<CAPutNode<double>>("CAPutDouble", ctx_,
                                                 pv_manager_);
    factory_.registerNodeType<CAPutNode<int>>("CAPutInt", ctx_, pv_manager_);
    factory_.registerNodeType<CAPutNode<std::string>>("CAPutString", ctx_,
                                                      pv_manager_);
    factory_.registerNodeType<PrintNode>("Print");

    factory_.registerBehaviorTreeFromFile(treePath);
    tree_ = factory_.createTree("MainTree", blackboard_);

    initialized_ = true;
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
