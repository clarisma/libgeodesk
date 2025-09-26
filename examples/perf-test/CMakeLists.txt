add_executable(perf-test main.cpp)
set_target_properties(perf-test PROPERTIES OUTPUT_NAME "perf-test-v2")
target_link_libraries(perf-test PRIVATE geodesk)
