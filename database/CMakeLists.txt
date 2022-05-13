#set( component_SOURCES ... ) # Add the source-files for the component here
# Optionally you can use file glob (uncomment the next line)
file( GLOB component_SOURCES *.cpp )

add_library( database ${component_SOURCES}  "inc/database/ConnectionPool.h" "inc/database/DBInterface.h" "inc/database/DBPostgres.h" "inc/database/QueryManager.h")

target_link_libraries(database PRIVATE libpqxx::pqxx plog grpc utils protocol fmt::fmt)

target_include_directories(database PUBLIC ${CMAKE_CURRENT_LIST_DIR}/inc)
