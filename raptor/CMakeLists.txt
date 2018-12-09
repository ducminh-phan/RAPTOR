add_library(raptor_lib
        config.hpp
        data_structure.cpp data_structure.hpp
        raptor.cpp raptor.hpp)
add_executable(raptor
        main.cpp
        config.hpp
        experiments.cpp experiments.hpp)

target_link_libraries(raptor raptor_lib)
target_link_libraries(raptor z)
set_target_properties(raptor PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)
set_target_properties(raptor_lib PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
target_link_libraries(raptor_lib Threads::Threads)
