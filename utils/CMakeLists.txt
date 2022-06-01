#set( component_SOURCES ... ) # Add the source-files for the component here
# Optionally you can use file glob (uncomment the next line)
file( GLOB component_SOURCES *.cpp )

add_library(utils ${component_SOURCES}  "inc/utils/catch_all.h" "inc/utils/FileManager.h" "inc/utils/invoke.h" "inc/utils/Mailer.h" "inc/utils/MailerInteface.h" "inc/utils/random.h" "inc/utils/scope_exit.h")

include_directories("inc/utils")

target_include_directories(utils PUBLIC ${CMAKE_CURRENT_LIST_DIR}/inc)
target_link_libraries(utils PRIVATE plog protocol fmt::fmt CURL::libcurl PUBLIC Boost::program_options)
