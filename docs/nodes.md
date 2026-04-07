# Nodes
## Standard Nodes

Since `bch-tree` is built upon **BehaviorTree.CPP v4.9**, it natively supports all standard node types (Control, Decorator, and Action nodes) included in the official library.

* **Key Standard Nodes**: `Sequence`, `Fallback`, `Parallel`, `RetryNode`, `Inverter`, etc.
* **Reference**: For detailed information on the logic and design of these nodes, please refer to the [Official BehaviorTree.CPP Documentation](https://www.behaviortree.dev/docs/category/nodes-library).

## EPICS Integration Nodes

The custom nodes in `bch-tree` are designed to bridge the Behavior Tree logic with EPICS Process Variables (PVs). These nodes are categorized by their function and the specific data types they handle (`Int`, `Double`, `String`) to ensure type-safe communication.

### CAGet Nodes
These Action nodes retrieve the current value from an EPICS PV and store it in the blackboard.

*   **Variants**: `CAGetInt`, `CAGetDouble`, `CAGetString`
*   **Port Configuration**:
    | Port          | Type                | Default | Description                                                                                                                          |
    | ------------- | ------------------- | ------- | ------------------------------------------------------------------------------------------------------------------------------------ |
    | `pv`          | `InputPort<string>` | -       | Name of the EPICS PV.                                                                                                                |
    | `timeout`     | `InputPort<int>`    | `1000`  | Timeout in milliseconds for connection and get operation.                                                                            |
    | `use_monitor` | `InputPort<bool>`   | `true`  | If `true`, the most recent value updated by the CA monitor is used. If `false`, an explicit get request is issued on each execution. |
    | `result`      | `OutputPort<T>`     | -       | Value read from the PV. The output type depends on the type of variant.                                                              |

### CAPut Nodes
These Action nodes write values to a specified EPICS PV.

*   **Variants**: `CAPutInt`, `CAPutDouble`, `CAPutString`
*   **Port Configuration**:
    | Port          | Type                | Default | Description                                                                                                                                                           |
    | ------------- | ------------------- | ------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
    | `pv`          | `InputPort<string>` | -       | Name of the EPICS PV.                                                                                                                                                 |
    | `value`       | `InputPort<T>`      | -       | Value to write to the PV. The type depends on the type of variant.                                                                                                    |
    | `timeout`     | `InputPort<int>`    | `1000`  | Timeout in milliseconds for connection and put operation.                                                                                                             |
    | `force_write` | `InputPort<bool>`   | `false` | If `true`, the value is always written even if it is the same as the last monitored value. If `false`, the write is skipped when the current PV value equals `value`. |


### CAMonitor Nodes
These nodes monitor an EPICS PV and automatically update the blackboard whenever the PV value changes.

*   **Variants**: `CAMonitorInt`, `CAMonitorDouble`, `CAMonitorString`
*   **Port Configuration**:
    | Port             | Type                | Default | Description                                                                                                                                   |
    | ---------------- | ------------------- | ------- | --------------------------------------------------------------------------------------------------------------------------------------------- |
    | `pv`             | `InputPort<string>` | -       | Name of the EPICS PV.                                                                                                                         |
    | `initial_fire`   | `InputPort<bool>`   | `true`  | If `true`, the node outputs the current PV value once at startup, even before any monitor event is received.                                  |
    | `queue_capacity` | `InputPort<int>`    | `60`    | Maximum number of CA monitor events to buffer internally. When the queue is full, the oldest samples are dropped.                             |
    | `result`         | `OutputPort<T>`     | -       | Value received from the EPICS CA monitor. One queued sample is published per successful tick (FIFO). The type depends on the type of variant. |

## Utility Nodes

### Print Node
The `Print` node is a utility Action node used for debugging and logging. It allows you to output messages or blackboard values directly to the standard output.

*   **Port Configuration**:
    | Port      | Type                | Default | Description                          |
    | --------- | ------------------- | ------- | ------------------------------------ |
    | `message` | `InputPort<string>` | -       | Message to print to standard output. |
