add_executable(gen_query
        main.cpp
        hub_labelling.cpp hub_labelling.hpp)

target_link_libraries(gen_query z)
set_target_properties(gen_query PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(gen_query Threads::Threads)
