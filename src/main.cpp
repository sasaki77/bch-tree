#include <cxxopts.hpp>
#include <iostream>

#include "bt_runner.h"
#include "logger.h"

int main(int argc, char** argv) {
    cxxopts::Options options("bch-tree-cli", "bch-tree CLI Runner");

    // clang-format off
  options.add_options()
    ("t,tree", "XML tree file", cxxopts::value<std::string>())
    ("log-level", "log level (info|warn|error)", cxxopts::value<std::string>()->default_value("info"))
    ("log-file", "log file path", cxxopts::value<std::string>()->default_value(""))
    ("h,help", "print usage");
    // clang-format on

    auto result = options.parse(argc, argv);
    if (result.count("help") || !result.count("tree")) {
        std::cout << options.help() << std::endl;
        return 2;
    }

    bchtree::Logger logger;
    logger.setLevel(result["log-level"].as<std::string>());
    auto logfile = result["log-file"].as<std::string>();
    if (!logfile.empty()) {
        logger.setFile(logfile);
    }

    bchtree::BTRunner runner;
    runner.setLogger(&logger);

    const std::string treePath = result["tree"].as<std::string>();
    int exit_code = runner.run(treePath);

    return exit_code;
}
