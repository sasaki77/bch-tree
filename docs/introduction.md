# Introduction

**bch-tree** is a C++ framework and command-line tool for executing **Behavior Trees**
to control **EPICS-based systems** from outside an IOC.

The project is built upon [**BehaviorTree.CPP**](https://github.com/BehaviorTree/BehaviorTree.CPP), a widely used library in robotics. **bch-tree** extends these capabilities to meet the requirements of control systems for accelerator facilities and experimental equipment.

## What are Behavior Trees?
Behavior Trees (BT) provide a powerful framework for designing complex system behaviors. They offer a flexible, modular, and reactive alternative to traditional Finite State Machines (FSM).

Since this program is based on BehaviorTree.CPP, please refer to the official documentation at [https://www.behaviortree.dev/](https://www.behaviortree.dev/) for in-depth details regarding node types, design philosophies, and core concepts.

## The Role of bch-tree
**bch-tree** brings the advantages of Behavior Trees into EPICS-based environments. It enables developers to implement sequence control and error handling for complex facilities in a way that is both intuitive and maintainable.
