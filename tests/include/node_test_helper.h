#include <behaviortree_cpp/bt_factory.h>

class NodeTestHelper {
   public:
    NodeTestHelper(std::shared_ptr<BT::BehaviorTreeFactory> factory);
    void registerNode();
    BT::NodeStatus runSingle(std::string xml,
                             std::chrono::milliseconds overall_timeout,
                             std::chrono::milliseconds step);

    template <typename T>
    bool getFromBB(const std::string& key, T& value) const {
        return blackboard_->get<T>(key, value);
    }

   private:
    std::shared_ptr<BT::BehaviorTreeFactory> factory_;
    BT::Tree tree_;
    std::shared_ptr<BT::Blackboard> blackboard_;
};
