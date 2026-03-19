#pragma once
#include <behaviortree_cpp/behavior_tree.h>

#include <deque>
#include <limits>

#include "epics/ca/ca_pv.h"
#include "epics/ca/ca_pv_manager.h"
#include "epics/types.h"

namespace bchtree {

// Event-driven monitor node with a thread-safe queue to avoid sample loss.
// Each tick returns SUCCESS for exactly one queued sample (FIFO).
template <typename T>
class CAMonitorNode : public BT::StatefulActionNode {
   public:
    static constexpr int kDefaultQueueCap = 60;  // bounded queue
    static constexpr bool kDefaultInitFire = true;

    CAMonitorNode(const std::string& name, const BT::NodeConfig& cfg,
                  std::shared_ptr<epics::ca::CAContextManager> ctx,
                  std::shared_ptr<epics::ca::PVManager> pv_manager)
        : BT::StatefulActionNode(name, cfg),
          ctx_(std::move(ctx)),
          pv_manager_(std::move(pv_manager)) {
        ctx_->EnsureAttached();
    }

    static BT::PortsList providedPorts() {
        using namespace BT;
        return {
            InputPort<std::string>("pv", "PV name"),
            InputPort<bool>(
                "initial_fire", kDefaultInitFire,
                " If true, the node outputs the current PV value once at the "
                "start, even if no monitor event has been received yet"),
            InputPort<int>("queue_capacity", kDefaultQueueCap,
                           "Maximum number of CA monitor events to buffer"),
            OutputPort<T>("result"),
        };
    }

    BT::NodeStatus onStart() override {
        cancelled_ = false;

        if (!getInput("pv", pv_name_)) {
            throw BT::RuntimeError(
                "CAMonitorNode: missing required input [pv]");
        }
        getInput("initial_fire", initial_fire_);
        getInput("queue_capacity", queue_capacity_);
        if (queue_capacity_ <= 0) queue_capacity_ = kDefaultQueueCap;

        if (!pv_) {
            pv_ = pv_manager_->Get(pv_name_);
            pv_->AddConnCB([this](bool c) { handleConnection(c); });
        }

        connected_.store(pv_->IsConnected());

        if (!monitor_cb_registered_) {
            pv_->AddMonitorCBAs<T>(
                [this](T sample) { handleValueEnqueue(std::move(sample)); });
            monitor_cb_registered_ = true;
        }

        if (!connected_.load()) {
            pv_->Connect();
            return BT::NodeStatus::RUNNING;
        }

        if (!initial_fire_) {
            return BT::NodeStatus::RUNNING;
        }

        // Arm drop-on-connect according to initial_fire BEFORE registering.
        // If we are already connected at this time and initial_fire==true,
        // we must drop the first monitor update to avoid duplication.
        if (!seen_any_connect_) {
            seen_any_connect_ = true;  // mark first connection seen
        }

        if (done_initial_fire_) {
            return BT::NodeStatus::RUNNING;
        }

        done_initial_fire_ = true;

        try {
            T v = pv_->GetAs<T>();
            setOutput("result", v);
        } catch (...) {
            // ignore; wait for first monitor update
        }
        return BT::NodeStatus::SUCCESS;
    }

    BT::NodeStatus onRunning() override {
        T out{};

        {
            std::lock_guard<std::mutex> lk(mtx_);
            if (queue_.empty()) {
                return BT::NodeStatus::RUNNING;
            }
            out = std::move(queue_.front());
            queue_.pop_front();
        }

        setOutput("result", out);
        return BT::NodeStatus::SUCCESS;
    }

    void onHalted() override {
        cancelled_ = true;
        std::lock_guard<std::mutex> lk(mtx_);
        queue_.clear();
    }

    // Non-copyable / movable
    CAMonitorNode(const CAMonitorNode&) = delete;
    CAMonitorNode& operator=(const CAMonitorNode&) = delete;
    CAMonitorNode(CAMonitorNode&&) noexcept = default;
    CAMonitorNode& operator=(CAMonitorNode&&) noexcept = default;

   private:
    void handleConnection(bool c) {
        connected_.store(c);
        if (!c) return;

        std::lock_guard<std::mutex> lk(mtx_);
        if (seen_any_connect_) {
            drop_next_after_connect_ = true;
            return;
        }

        // First time we see 'connected' (initial connect):
        seen_any_connect_ = true;
        done_initial_fire_ = true;

        // If initial_fire==true and we didn't arm earlier, arm now.
        if (!initial_fire_) {
            drop_next_after_connect_ = true;
        }
    }

    void handleValueEnqueue(T sample) {
        if (cancelled_) return;
        {
            std::lock_guard<std::mutex> lk(mtx_);

            // Drop the very first monitor event after (re)connect if armed.
            if (drop_next_after_connect_) {
                drop_next_after_connect_ = false;
                return;
            }

            enqueueUnlocked_(std::move(sample));
        }
        emitWakeUpSignal();  // wake the tree for prompt delivery
    }

    void enqueueUnlocked_(T sample) {
        // Keep queue bounded; drop oldest to preserve latest real-time values
        if (queue_.size() >= static_cast<size_t>(queue_capacity_)) {
            queue_.pop_front();
            ++dropped_oldest_;
        }
        queue_.push_back(std::move(sample));
    }

   private:
    // EPICS handles
    std::shared_ptr<epics::ca::CAPV> pv_;
    std::shared_ptr<epics::ca::CAContextManager> ctx_;
    std::shared_ptr<epics::ca::PVManager> pv_manager_;

    // Inputs
    std::string pv_name_;
    bool initial_fire_{kDefaultInitFire};
    int queue_capacity_{kDefaultQueueCap};

    // State
    std::atomic<bool> cancelled_{false};
    std::atomic<bool> connected_{false};
    std::atomic<bool> done_initial_fire_{false};
    bool monitor_cb_registered_{false};

    // Connection bookkeeping for “ignore initial”
    // have we ever seen 'connected=true' ?
    bool seen_any_connect_{false};
    // drop exactly one update after (re)connect
    bool drop_next_after_connect_{false};

    // Queue and timing
    std::mutex mtx_;
    std::deque<T> queue_;
    size_t dropped_oldest_{0};
};

}  // namespace bchtree
