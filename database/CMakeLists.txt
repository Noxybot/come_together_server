#set( component_SOURCES ... ) # Add the source-files for the component here
# Optionally you can use file glob (uncomment the next line)
file( GLOB component_SOURCES *.cpp )

add_library( database ${component_SOURCES}  "inc/database/ConnectionPool.h" "inc/database/DBInterface.h" "inc/database/DBPostgres.h" "inc/database/QueryManager.h" "DBMongo.cpp" "inc/database/DBMongo.h")

target_link_libraries(database PRIVATE libpqxx::pqxx plog grpc utils protocol fmt::fmt
                        PUBLIC Boost::program_options  mongo::mongocxx_static jwt-cpp::jwt-cpp picojson::picojson)

target_include_directories(database PUBLIC ${CMAKE_CURRENT_LIST_DIR}/inc)
