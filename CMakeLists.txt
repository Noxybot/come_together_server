cmake_minimum_required(VERSION 3.19)

project(ComeTogether VERSION 1.0)
set (CMAKE_CXX_STANDARD 17)

include(cmake/conan_config.cmake)

# Ignore warnings about missing pdb
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /ignore:4099")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} /ignore:4099")
set(CMAKE_STATIC_LINKER_FLAGS "${CMAKE_STATIC_LINKER_FLAGS} /ignore:4099")

add_subdirectory(plog)
add_subdirectory(come_together_common/proto)
add_subdirectory(core)
add_subdirectory(database)
add_subdirectory(network)
add_subdirectory(tests)
add_subdirectory(utils)

add_executable(server main.cpp Server.cpp config.ini)
install(FILES config.ini DESTINATION etc/cfg COMPONENT config)

target_link_libraries(server PRIVATE plog core database utils)
