add_library(networkHelper INTERFACE)

target_include_directories(networkHelper INTERFACE include)

target_link_libraries(networkHelper INTERFACE utils)
