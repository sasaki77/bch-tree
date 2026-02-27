#include "bt_factory_host.h"

#include <stdexcept>

#include "actions/caget_node.h"
#include "actions/camonitor_node.h"
#include "actions/caput_node.h"
#include "actions/print_node.h"

namespace bchtree {

void BTFactoryHost::registerBuiltinNodes(
    std::shared_ptr<epics::ca::CAContextManager> ctx,
    std::shared_ptr<epics::ca::PVManager> pv_manager) {
    factory_.registerNodeType<CAGetNode<double>>("CAGetDouble", ctx,
                                                 pv_manager);
    factory_.registerNodeType<CAGetNode<int>>("CAGetInt", ctx, pv_manager);
    factory_.registerNodeType<CAGetNode<std::string>>("CAGetString", ctx,
                                                      pv_manager);

    factory_.registerNodeType<CAPutNode<double>>("CAPutDouble", ctx,
                                                 pv_manager);
    factory_.registerNodeType<CAPutNode<int>>("CAPutInt", ctx, pv_manager);
    factory_.registerNodeType<CAPutNode<std::string>>("CAPutString", ctx,
                                                      pv_manager);

    factory_.registerNodeType<CAMonitorNode<double>>("CAMonitorDouble", ctx,
                                                     pv_manager);
    factory_.registerNodeType<CAMonitorNode<int>>("CAMonitorInt", ctx,
                                                  pv_manager);
    factory_.registerNodeType<CAMonitorNode<std::string>>("CAMonitorString",
                                                          ctx, pv_manager);

    factory_.registerNodeType<PrintNode>("Print");
}

void BTFactoryHost::addPluginPath(std::string path) {
    std::lock_guard<std::mutex> lk(mtx_);
    plugin_paths_.emplace_back(std::move(path));
    prepared_ = false;  // new plugin requires re-prepare
}

void BTFactoryHost::registerTreeFromFile(const std::string& xml_file_path) {
    factory_.registerBehaviorTreeFromFile(xml_file_path);
}

void BTFactoryHost::registerTreeFromText(const std::string& xml_text) {
    factory_.registerBehaviorTreeFromText(xml_text);
}

void BTFactoryHost::prepareOnce() {
    std::lock_guard<std::mutex> lk(mtx_);
    if (prepared_) return;

    // Load plugins. Each plugin must export BT_REGISTER_NODES(factory).
    for (const auto& so : plugin_paths_) {
        factory_.registerFromPlugin(so);
    }

    prepared_ = true;
}

std::vector<std::string> BTFactoryHost::registeredTreeIDs() const {
    return factory_.registeredBehaviorTrees();
}

BT::Tree BTFactoryHost::createTree(const std::string& tree_id,
                                   const BT::Blackboard::Ptr& blackboard) {
    ensurePrepared_();
    // Instantiate the tree by ID from previously registered definitions.
    // This returns a BT::Tree value object that owns the instance.
    auto tree = factory_.createTree(tree_id, blackboard);

    return tree;
}

void BTFactoryHost::ensurePrepared_() {
    if (!prepared_) {
        prepareOnce();
    }
}

}  // namespace bchtree
