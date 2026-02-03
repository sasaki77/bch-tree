#pragma once
#include <behaviortree_cpp/behavior_tree.h>

namespace bchtree {
class PrintNode : public BT::SyncActionNode {
   public:
    PrintNode(const std::string& name, const BT::NodeConfig& config)
        : BT::SyncActionNode(name, config) {}

    static BT::PortsList providedPorts();
    BT::NodeStatus tick() override;
};

}  // namespace bchtree
