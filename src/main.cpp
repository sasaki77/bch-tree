#include <chrono>
#include <cxxopts.hpp>
#include <iostream>

#include "bt_runner.h"
#include "logger.h"

enum ExitCode {
    OK = 0,            // Tree SUCCESS
    TREE_FAILURE = 1,  // Tree FAILURE
    USAGE_ERROR = 2,   // argument error (--tree missing etc.)
};

int main(int argc, char** argv) {
    cxxopts::Options options("bch-tree-cli", "bch-tree CLI Runner");

    // clang-format off
    options.add_options()
      ("t,tree", "XML tree file", cxxopts::value<std::string>())
      ("log-level", "log level (info|warn|error|debug)", cxxopts::value<std::string>()->default_value("info"))
      ("log-file", "log file path", cxxopts::value<std::string>()->default_value(""))
      ("print-tree", "print tree", cxxopts::value<bool>()->default_value("false")->implicit_value("true"))
      ("sleep-time", "sleep time for tick in msec", cxxopts::value<int>()->default_value("10"))
      ("h,help", "print usage");
    // clang-format on

    auto result = options.parse(argc, argv);
    if (result.count("help") || !result.count("tree")) {
        std::cout << options.help() << std::endl;
        return USAGE_ERROR;
    }

    auto logger = std::make_shared<bchtree::Logger>();
    auto log_level = result["log-level"].as<std::string>();
    logger->setLevel(log_level);

    auto logfile = result["log-file"].as<std::string>();
    if (!logfile.empty()) {
        logger->setFile(logfile);
    }

    auto ctx = std::make_shared<bchtree::epics::ca::CAContextManager>();
    ctx->Init();
    auto pv_manager = std::make_shared<bchtree::epics::ca::PVManager>(ctx);

    bchtree::BTRunner runner(ctx, pv_manager);
    runner.SetLogger(logger);
    if (log_level == "debug") {
        runner.UseRunnerLogger();
    }

    const std::string treePath = result["tree"].as<std::string>();
    runner.RegisterTreeFromFile(treePath);

    if (result["print-tree"].as<bool>()) {
        runner.PrintTree();
        return OK;
    }

    auto sleep_time_arg = result["sleep-time"].as<int>();
    auto sleep_time = std::chrono::milliseconds(sleep_time_arg);
    bool success = runner.Run(sleep_time);
    if (success) {
        return OK;
    }

    return TREE_FAILURE;
}
