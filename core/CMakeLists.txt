#set( component_SOURCES ... ) # Add the source-files for the component here
# Optionally you can use file glob (uncomment the next line)
file( GLOB component_SOURCES *.cpp )

add_library(core ${component_SOURCES}  "inc/core/PlacesStorageImpl.h" "inc/core/PlacesStorageInterface.h" "inc/core/PushSender.h" "inc/core/UserInstance.h" "inc/core/UserPresenceInterface.h" "inc/core/UserStorageImpl.h" "inc/core/UserStorageInterface.h" "inc/core/User.h")

target_link_libraries(core PRIVATE plog database)
