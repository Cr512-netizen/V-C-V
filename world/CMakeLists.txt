add_library(nuc_world registry.cpp)
target_include_directories(nuc_world PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(nuc_world PUBLIC nlohmann_json::nlohmann_json)
