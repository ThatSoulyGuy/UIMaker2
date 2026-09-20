cmake_minimum_required(VERSION 3.16)
project(uim2grid LANGUAGES CXX)
set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
find_package(Qt6 REQUIRED COMPONENTS Widgets Test)

set(SRC_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../../src)
file(GLOB_RECURSE PROJ_SRC ${SRC_ROOT}/*.cpp ${SRC_ROOT}/*.hpp)
list(REMOVE_ITEM PROJ_SRC ${SRC_ROOT}/main.cpp)
# MainWindow needs the .ui; skip the whole app layer, the harness does not use it.
list(REMOVE_ITEM PROJ_SRC ${SRC_ROOT}/app/MainWindow.cpp ${SRC_ROOT}/app/MainWindow.hpp)

add_executable(uim2grid main.cpp ${PROJ_SRC})
target_include_directories(uim2grid PRIVATE ${SRC_ROOT})
target_link_libraries(uim2grid PRIVATE Qt6::Widgets)
target_compile_options(uim2grid PRIVATE -O2 -DNDEBUG)

add_executable(uim2smoke smoke.cpp ${PROJ_SRC})
target_include_directories(uim2smoke PRIVATE ${SRC_ROOT})
target_link_libraries(uim2smoke PRIVATE Qt6::Widgets Qt6::Test)
target_compile_options(uim2smoke PRIVATE -O2 -DNDEBUG)
