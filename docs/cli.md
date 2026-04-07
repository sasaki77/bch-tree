# CLI Usage

The `bch-tree-cli` is a command-line tool designed to load, execute, and debug Behavior Trees within the `bch-tree` framework.

## Basic Syntax

To run a behavior tree, execute the following command:

```bash
bch-tree-cli [OPTIONS]
```

### Command Line Options

| Option                | Description                                                  | Default    |
| :-------------------- | :----------------------------------------------------------- | :--------- |
| `-t`, `--tree`        | Path to the XML tree file to be executed.                    | (Required) |
| `--tree-id`           | Specify which Tree ID to use as the root from the XML.       | `MainTree` |
| `-s`, `--set`         | Set a Global Blackboard entry (`key=value`).                 | `""`       |
| `--sleep-time`        | Interval between tree ticks in milliseconds.                 | `10`       |
| `--print-tree`        | Prints the structure of the loaded tree to the console.      | -          |
| `--print-tree-ids`    | Prints all Tree IDs found in the XML file.                   | -          |
| `--output-models`     | Outputs the tree nodes model XML for Groot2.                 | -          |
| `--output-xsd`        | Outputs the XML Schema Definition (XSD) for tree validation. | -          |
| `--log-level-console` | Set the logging level for the console.                       | `info`     |
| `--log-level-file`    | Set the logging level for the log file.                      | `info`     |
| `--log-file`          | Path to the output log file.                                 | `""`       |
| `-h`, `--help`        | Display the help message.                                    | -          |

## Feature Details
### Global Blackboard (`-s`, `--set`)
The `-s` option populates the **Global Blackboard**. These entries are accessible from any part of the tree or subtrees using the **`@` prefix** (e.g., `{@value}`).

- **Reference**: For technical details, see the [BehaviorTree.CPP Official Tutorial on Global Blackboards](https://www.behaviortree.dev/docs/tutorial-advanced/tutorial_16_global_blackboard).

### Groot2 Integration (`--output-models`)
To visualize custom nodes in **Groot2**, you must import their definitions.
1. Generate the model file: `bch-tree-cli --output-models > models.xml`
2. Open Groot2 and Import the `models.xml` file.

### XML Validation (`--output-xsd`)
The **`--output-xsd`** option generates an XML Schema Definition (XSD).
This allows you to validate your Behavior Tree XML files and enables auto-completion/error-checking in modern XML editors.

- **Practical Example**: For a concrete example of a valid XML structure used within this framework, please refer to [**`examples/vacuum.xml`**](https://github.com/sasaki77/bch-tree/blob/main/examples/vacuum.xml).

### Tree Inspection (`--print-tree`, `--print-tree-ids`)
The CLI provides options to inspect the structure and metadata of your Behavior Trees.

- **`--print-tree`**: Prints the hierarchical structure of the loaded Behavior Tree to the console.
- **`--print-tree-ids`**: Lists all available tree identifiers (Tree IDs) defined within the XML file.

### Logging
`bch-tree-cli` supports simultaneous logging to both the console and a file. You can configure the log level for each output independently.

- **Log Levels**: The following levels are available for both console and file logging:
    - `trace`, `debug`, `info`, `warn`, `error`, `critical`, `off`
- **Configuration**:
    - Use **`--log-file`** to specify the path for the log file (e.g., `execution.log`).
    - Adjust log level via `--log-level-console` and `--log-level-file`.
