.. bch-tree documentation master file, created by
   sphinx-quickstart on Mon Apr  6 14:33:04 2026.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

bch-tree
======================

bch-tree is a C++ framework and command-line tool for executing Behavior Trees
to control EPICS-based systems from outside an IOC.
It is intended to orchestrate EPICS PVs using Behavior Trees as high-level control logic.
It is built upon the `BehaviorTree.CPP <https://www.behaviortree.dev/>`_ library.

Links
-----
* `Source (GitHub) <https://github.com/sasaki77/bch-tree>`_

Features
--------
* **EPICS Integration**: Specifically designed to work within EPICS-based control systems.
* **BehaviorTree.CPP**: Leverages the features of the BehaviorTree.CPP library.
* **Groot2 Support**: Provides an option for creating and visualizing a tree via **Groot2**.


.. toctree::
   :maxdepth: 2
   :caption: Documentation

   introduction
   installation
   cli
   nodes
   development
