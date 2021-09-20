add_executable(test_dm_sim DmSimRunnerTest.cpp)
target_link_libraries(test_dm_sim PRIVATE xacc-dm-sim-nvidia-omp)
target_include_directories(test_dm_sim PRIVATE ../..)