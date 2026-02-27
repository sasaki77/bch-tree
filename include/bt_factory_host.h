#pragma once

#include <behaviortree_cpp/blackboard.h>
#include <behaviortree_cpp/bt_factory.h>

#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "epics/ca/ca_context_manager.h"
#include "epics/ca/ca_pv_manager.h"

namespace bchtree {

class BTFactoryHost final {
   public:
    BTFactoryHost() = default;
    BTFactoryHost(const BTFactoryHost&) = delete;
    BTFactoryHost& operator=(const BTFactoryHost&) = delete;

    // ---- Configuration phase (to be called before creating trees) ----

    // Direct node registration API for the app to call at init.
    void registerBuiltinNodes(std::shared_ptr<epics::ca::CAContextManager> ctx,
                              std::shared_ptr<epics::ca::PVManager> pv_manager);

    // Add a shared library path to be loaded via registerFromPlugin().
    // The library must export BT_REGISTER_NODES(factory).
    void addPluginPath(std::string path);

    // Register BehaviorTree definitions (XML) into the factory (by file or
    // text). These APIs do not instantiate trees; they only register
    // definitions by ID.
    void registerTreeFromFile(const std::string& xml_file_path);
    void registerTreeFromText(const std::string& xml_text);

    // Apply all registrars and plugins exactly once (idempotent).
    void prepareOnce();

    // ---- Query / Creation phase ----

    // Return registered tree IDs. Useful to validate expected IDs exist.
    std::vector<std::string> registeredTreeIDs() const;

    // Generate XSD from the factory.
    std::string writeTreeXSD() const;

    // Create a Tree instance from a registered ID and a blackboard.
    // This ensures the factory is prepared (prepareOnce) before instantiation.
    BT::Tree createTree(
        const std::string& tree_id,
        const BT::Blackboard::Ptr& blackboard = BT::Blackboard::create());

   private:
    // Ensure preparation has been performed. Thread-safe and idempotent.
    void ensurePrepared_();

    // Internal state (hidden from clients)
    BT::BehaviorTreeFactory factory_;
    std::vector<std::string> plugin_paths_;
    bool prepared_{false};

    // To make prepareOnce thread-safe in case of concurrent callers.
    mutable std::mutex mtx_;
};

}  // namespace bchtree
