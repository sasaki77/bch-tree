#pragma once
#include <behaviortree_cpp/bt_factory.h>
#include <behaviortree_cpp/loggers/abstract_logger.h>

#include <memory>
#include <string>

#include "epics/ca/ca_context_manager.h"
#include "epics/ca/ca_pv_manager.h"
#include "logger.h"

namespace bchtree {

class RunnerLogger : public BT::StatusChangeLogger {
   public:
    RunnerLogger(const BT::Tree& tree, std::shared_ptr<Logger> logger);
    ~RunnerLogger() override;

    RunnerLogger(const RunnerLogger&) = delete;
    RunnerLogger& operator=(const RunnerLogger&) = delete;
    RunnerLogger(RunnerLogger&&) = delete;
    RunnerLogger& operator=(RunnerLogger&&) = delete;

    virtual void flush() override;

   private:
    virtual void callback(BT::Duration timestamp, const BT::TreeNode& node,
                          BT::NodeStatus prev_status,
                          BT::NodeStatus status) override;

    std::shared_ptr<Logger> logger_;
};

class BTRunner {
   public:
    explicit BTRunner(std::shared_ptr<epics::ca::CAContextManager> ctx,
                      std::shared_ptr<epics::ca::PVManager> pv_manager)
        : ctx_(std::move(ctx)), pv_manager_(std::move(pv_manager)) {}

    bool Run(
        std::chrono::milliseconds sleep_time = std::chrono::milliseconds(10));
    void PrintTree();
    void SetLogger(std::shared_ptr<Logger> logger);
    void UseRunnerLogger();
    void RegisterTreeFromFile(const std::string& treePath);

   private:
    std::shared_ptr<Logger> logger_;
    BT::BehaviorTreeFactory factory_;
    BT::Tree tree_;
    std::shared_ptr<BT::Blackboard> blackboard_;

    std::shared_ptr<epics::ca::CAContextManager> ctx_;
    std::shared_ptr<epics::ca::PVManager> pv_manager_;

    bool initialized_{false};
    bool use_runner_logger_{false};
    std::unique_ptr<RunnerLogger> runner_logger_;
};

}  // namespace bchtree
