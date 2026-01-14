#pragma once
#include <behaviortree_cpp/bt_factory.h>

#include <memory>
#include <string>

#include "logger.h"

namespace bchtree {
class BTRunner {
   public:
    int run(const std::string& treePath);
    void setLogger(Logger* logger);

   private:
    Logger* logger_{nullptr};
    std::unique_ptr<BT::Tree> tree_;  // 実体はファクトリ生成後に保持
    std::shared_ptr<BT::Blackboard> blackboard_;
};
}  // namespace bchtree
