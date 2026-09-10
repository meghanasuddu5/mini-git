cmake_minimum_required(VERSION 3.20)
project(minigit
    VERSION 0.1.0
    DESCRIPTION "A Git-like version control system from scratch"
    LANGUAGES CXX
)

# ─── C++20 Standard ──────────────────────────────────────────────────────────
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ─── Compiler Warnings ──────────────────────────────────────────────────────
if(MSVC)
    add_compile_options(/W4 /permissive-)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# ─── Include Paths ───────────────────────────────────────────────────────────
include_directories(${PROJECT_SOURCE_DIR}/include)

# ─── Source Files ────────────────────────────────────────────────────────────
set(MINIGIT_LIB_SOURCES
    src/utils/filesystem.cpp
    src/repository/repository.cpp
    src/cli/cli.cpp
    src/object/hash.cpp
    src/object/object.cpp
    src/object/object_store.cpp
    src/index/index.cpp
    src/index/ignore.cpp
    src/object/tree.cpp
    src/object/commit.cpp
    src/repository/ref_manager.cpp
    src/repository/checkout.cpp
    src/repository/graph_analyzer.cpp
    src/repository/merge.cpp
    src/utils/diff.cpp
)

# Library target (used by both the executable and tests)
add_library(minigit_lib STATIC ${MINIGIT_LIB_SOURCES})
target_include_directories(minigit_lib PUBLIC ${PROJECT_SOURCE_DIR}/include)

# ─── Main Executable ────────────────────────────────────────────────────────
add_executable(minigit src/main.cpp)
target_link_libraries(minigit PRIVATE minigit_lib)

# ─── Testing ─────────────────────────────────────────────────────────────────
enable_testing()

add_executable(test_init tests/test_init.cpp)
target_link_libraries(test_init PRIVATE minigit_lib)
add_test(NAME test_init COMMAND test_init)

add_executable(test_cli tests/test_cli.cpp)
target_link_libraries(test_cli PRIVATE minigit_lib)
add_test(NAME test_cli COMMAND test_cli)

add_executable(test_hash tests/test_hash.cpp)
target_link_libraries(test_hash PRIVATE minigit_lib)
add_test(NAME test_hash COMMAND test_hash)

add_executable(test_object_store tests/test_object_store.cpp)
target_link_libraries(test_object_store PRIVATE minigit_lib)
add_test(NAME test_object_store COMMAND test_object_store)

add_executable(test_index tests/test_index.cpp)
target_link_libraries(test_index PRIVATE minigit_lib)
add_test(NAME test_index COMMAND test_index)

add_executable(test_tree tests/test_tree.cpp)
target_link_libraries(test_tree PRIVATE minigit_lib)
add_test(NAME test_tree COMMAND test_tree)

add_executable(test_commit tests/test_commit.cpp)
target_link_libraries(test_commit PRIVATE minigit_lib)
add_test(NAME test_commit COMMAND test_commit)

add_executable(test_ignore tests/test_ignore.cpp)
target_link_libraries(test_ignore PRIVATE minigit_lib)
add_test(NAME test_ignore COMMAND test_ignore)

add_executable(test_ref_manager tests/test_ref_manager.cpp)
target_link_libraries(test_ref_manager PRIVATE minigit_lib)
add_test(NAME test_ref_manager COMMAND test_ref_manager)

add_executable(test_checkout tests/test_checkout.cpp)
target_link_libraries(test_checkout PRIVATE minigit_lib)
add_test(NAME test_checkout COMMAND test_checkout)

add_executable(test_diff tests/test_diff.cpp)
target_link_libraries(test_diff PRIVATE minigit_lib)
add_test(NAME test_diff COMMAND test_diff)

add_executable(test_merge tests/test_merge.cpp)
target_link_libraries(test_merge PRIVATE minigit_lib)
add_test(NAME test_merge COMMAND test_merge)

add_executable(test_graph_analyzer tests/test_graph_analyzer.cpp)
target_link_libraries(test_graph_analyzer PRIVATE minigit_lib)
add_test(NAME test_graph_analyzer COMMAND test_graph_analyzer)

# ─── Benchmarking ────────────────────────────────────────────────────────────
add_executable(benchmark tests/benchmark.cpp)
target_link_libraries(benchmark PRIVATE minigit_lib)


