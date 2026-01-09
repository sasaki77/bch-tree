#include <behaviortree_cpp/xml_parsing.h>

#include "bt_runner.h"

namespace
{
    int map_status_to_exit_code(BT::NodeStatus status)
    {
        switch (status)
        {
        case BT::NodeStatus::SUCCESS:
            return 0;
        case BT::NodeStatus::FAILURE:
            return 1;
        default:
            return 2;
        }
    }
}

namespace beechtree
{

    int BTRunner::run(const std::string &treePath)
    {
        BT::BehaviorTreeFactory factory;
        blackboard_ = BT::Blackboard::create();

        BT::Tree tree = factory.createTreeFromFile(treePath, blackboard_);
        tree_ = std::make_unique<BT::Tree>(std::move(tree));

        if (logger_)
        {
            logger_->info("Start Tree: " + treePath);
        }

        const BT::NodeStatus status = tree_->tickWhileRunning();

        if (logger_)
        {
            logger_->info(std::string("End Tree: status=") + toStr(status));
        }

        return map_status_to_exit_code(status);
    }

    void BTRunner::setLogger(Logger *logger) { logger_ = logger; }
}
