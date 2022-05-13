cmake_minimum_required(VERSION 3.19.3)

project(ComeTogether VERSION 1.0)
set (CMAKE_CXX_STANDARD 17)

include(cmake/conan_config.cmake)
include(cmake/grpc_extras.cmake)

set(_gRPC_PROTO_GENS_DIR ${CMAKE_BINARY_DIR}/gens)
file(MAKE_DIRECTORY ${_gRPC_PROTO_GENS_DIR})

add_subdirectory(plog)
add_subdirectory(core)
add_subdirectory(database)
add_subdirectory(network)
add_subdirectory(tests)
add_subdirectory(utils)

protobuf_generate_grpc_cpp(GRPC_SRCS GRPC_HDRS
    come_together_common/proto/come_together.proto)
message("test1 ${GRPC_SRCS} ${GRPC_HDRS}")
add_executable(server main.cpp ${GRPC_SRCS} ${GRPC_HDRS})
target_include_directories(server PUBLIC "${_gRPC_PROTO_GENS_DIR}")

target_link_libraries(server PRIVATE plog core database gRPC::grpc++_unsecure)
