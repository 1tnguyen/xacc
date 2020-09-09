add_executable(placement_run placement_run.cpp)
target_link_libraries(placement_run PRIVATE xacc)
target_compile_definitions(placement_run PRIVATE QASM_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/resources")
