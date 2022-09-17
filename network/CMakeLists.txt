#set( component_SOURCES ... ) # Add the source-files for the component here
# Optionally you can use file glob (uncomment the next line)
file( GLOB component_SOURCES *.cpp )

add_library( network ${component_SOURCES}  "inc/network/CommonCallData.h" "inc/network/MainEndpointImpl.h" "inc/network/QueueEvents.h" "inc/network/VerificationServiceImpl.h")
include_directories("inc/network")

target_link_libraries(network PRIVATE core database plog utils protocol)

target_include_directories(network PUBLIC ${CMAKE_CURRENT_LIST_DIR}/inc)
