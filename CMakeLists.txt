cmake_minimum_required(VERSION 3.23)
project(bch-tree LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(BehaviorTree REQUIRED)
find_package(spdlog REQUIRED)
find_package(cxxopts REQUIRED)

add_library(bchtree
    src/bt_runner.cpp
    src/logger.cpp
)
target_include_directories(bchtree PUBLIC include)
target_link_libraries(bchtree PUBLIC
    BT::behaviortree_cpp

    spdlog::spdlog
)

add_executable(bch-tree-cli src/main.cpp)
target_link_libraries(bch-tree-cli PRIVATE bchtree cxxopts::cxxopts spdlog::spdlog)

install(TARGETS bch-tree-cli RUNTIME DESTINATION bin)
