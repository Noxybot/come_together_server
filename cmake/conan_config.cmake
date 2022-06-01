list(APPEND CMAKE_MODULE_PATH ${CMAKE_BINARY_DIR})
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_BINARY_DIR})

if(NOT EXISTS "${CMAKE_BINARY_DIR}/conan.cmake")
  message(STATUS "Downloading conan.cmake from https://github.com/conan-io/cmake-conan")
  file(DOWNLOAD "https://raw.githubusercontent.com/conan-io/cmake-conan/0.18.1/conan.cmake"
                "${CMAKE_BINARY_DIR}/conan.cmake"
                TLS_VERIFY ON)
endif()

include(${CMAKE_BINARY_DIR}/conan.cmake)

conan_cmake_configure(REQUIRES openssl/1.1.1n libpqxx/7.7.3 grpc/1.45.2 protobuf/3.20.0 fmt/8.1.1
                      libcurl/7.83.0 jwt-cpp/0.6.0 picojson/cci.20210117 mongo-cxx-driver/3.6.6 boost/1.78.0
                      GENERATORS cmake_find_package
                      OPTIONS libpq:with_openssl=True)

conan_cmake_autodetect(settings)

conan_cmake_install(PATH_OR_REFERENCE .
                    BUILD missing
                    REMOTE conancenter
                    SETTINGS ${settings})

find_package(libpqxx)
find_package(grpc)
find_package(protobuf)
find_package(fmt)
find_package(CURL)
find_package(jwt-cpp)
find_package(picojson)
find_package(mongocxx)
find_package(Boost)
