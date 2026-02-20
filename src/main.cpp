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
      ("log-level-console", "(trace|debug|info|warn|error|critical|off)", cxxopts::value<std::string>()->default_value("info"))
      ("log-level-file", "(trace|debug|info|warn|error|critical|off)", cxxopts::value<std::string>()->default_value("info"))
      ("log-file", "log file path", cxxopts::value<std::string>()->default_value(""))
      ("print-tree", "print tree", cxxopts::value<bool>()->default_value("false")->implicit_value("true"))
      ("s,set", "Set global blackboard entry (key=value). Repeatable.", cxxopts::value<std::vector<std::string>>()->default_value({}))
      ("sleep-time", "sleep time for tick in msec", cxxopts::value<int>()->default_value("10"))
      ("h,help", "print usage");
    // clang-format on

    auto result = options.parse(argc, argv);

    if (result.count("help") || !result.count("tree")) {
        std::cout << options.help() << std::endl;
        return USAGE_ERROR;
    }

    auto logger = std::make_shared<bchtree::Logger>();

    // Read per-sink levels from CLI (use defaults if not provided)
    const auto console_level = result["log-level-console"].as<std::string>();
    const auto file_level = result["log-level-file"].as<std::string>();

    logger->setConsoleLevel(console_level);
    logger->setFileLevel(file_level);

    auto logfile = result["log-file"].as<std::string>();
    if (!logfile.empty()) {
        logger->setFile(logfile);
    }

    auto ctx = std::make_shared<bchtree::epics::ca::CAContextManager>();
    ctx->Init();
    auto pv_manager = std::make_shared<bchtree::epics::ca::PVManager>(ctx);

    bchtree::BTRunner runner(ctx, pv_manager);
    runner.SetLogger(logger);

    if (console_level == "debug" || file_level == "debug") {
        runner.UseRunnerLogger();
    }

    // Parse --set key=value pairs and pass them to BTRunner BEFORE
    // RegisterTreeFromFile().
    if (result.count("set")) {
        const auto pairs = result["set"].as<std::vector<std::string>>();
        for (const auto& kv : pairs) {
            // Expected format: key=value (no spaces)
            const auto pos = kv.find('=');
            if (pos == std::string::npos) {
                logger->error(std::string("Invalid --set '") + kv +
                              "'. Expected key=value.");
                return USAGE_ERROR;
            }
            const std::string key = kv.substr(0, pos);
            const std::string val = kv.substr(pos + 1);
            // Note: keys are plain strings (e.g., "@head", "mode", etc.)
            runner.SetGlobalBB(key, val);
        }
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
