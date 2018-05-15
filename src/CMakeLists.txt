add_library(raptor_lib
        csv_reader.hpp
        data_structure.cpp data_structure.hpp
        utilities.cpp utilities.hpp
        raptor.cpp raptor.hpp
        )
add_executable(raptor main.cpp)

target_link_libraries(raptor raptor_lib)
set_target_properties(raptor PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)
set_target_properties(raptor_lib PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build)
