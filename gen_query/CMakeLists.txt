add_executable(gen_query
        main.cpp
        hub_labelling.cpp hub_labelling.hpp)

target_link_libraries(gen_query z)
set_target_properties(gen_query PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)
