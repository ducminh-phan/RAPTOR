add_library(raptor_lib
        data_structure.cpp data_structure.hpp
        raptor.cpp raptor.hpp
        experiments.cpp experiments.hpp)
add_executable(raptor main.cpp)

target_link_libraries(raptor raptor_lib)
target_link_libraries(raptor z)
set_target_properties(raptor PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)
set_target_properties(raptor_lib PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)
