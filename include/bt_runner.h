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

class BTFactoryHost;

class BTRunner {
   public:
    explicit BTRunner(BTFactoryHost& host) : host_(host) {}

    using BBInitMap = std::unordered_map<std::string, std::string>;

    bool Run(const std::string& tree_id, const BBInitMap& bb_map,
             const std::chrono::milliseconds sleep_time =
                 std::chrono::milliseconds(10));
    void PrintTree(const std::string& tree_id = "MainTree");
    void SetLogger(std::shared_ptr<Logger> logger);
    void UseRunnerLogger();

   private:
    static BT::Blackboard::Ptr MakeBlackboard(
        const std::unordered_map<std::string, std::string>& bb_map);

    std::shared_ptr<Logger> logger_;
    BTFactoryHost& host_;

    bool use_runner_logger_{false};
    std::unique_ptr<RunnerLogger> runner_logger_;
};

}  // namespace bchtree
