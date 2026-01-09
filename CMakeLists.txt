cmake_minimum_required(VERSION 3.23)
project(beech-tree LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(BehaviorTree REQUIRED)
find_package(spdlog REQUIRED)
find_package(cxxopts REQUIRED)

add_library(beechtree
    src/bt_runner.cpp
    src/logger.cpp
)
target_include_directories(beechtree PUBLIC include)
target_link_libraries(beechtree PUBLIC
    BT::behaviortree_cpp

    spdlog::spdlog
)

add_executable(beech-tree-cli src/main.cpp)
target_link_libraries(beech-tree-cli PRIVATE beechtree cxxopts::cxxopts spdlog::spdlog)

install(TARGETS beech-tree-cli RUNTIME DESTINATION bin)
