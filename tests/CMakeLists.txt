#set( component_SOURCES ... ) # Add the source-files for the component here
# Optionally you can use file glob (uncomment the next line)
file( GLOB component_SOURCES *.cpp )

add_library( tests ${component_SOURCES} )

target_link_libraries(tests PRIVATE plog gRPC::grpc++)
