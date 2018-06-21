add_executable(tests
        test.cpp
        test_data_structure.cpp)

target_link_libraries(tests Catch)
target_link_libraries(tests raptor_lib)
target_link_libraries(tests z)
set_target_properties(tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)
