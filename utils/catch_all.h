#pragma once
#include <plog/Log.h>
#define CATCH_ALL(ret) catch (pqxx::sql_error const &e)\
{\
    PLOG_ERROR\
    << "Database error: " << e.what() << std::endl\
    << "Query was: " << e.query();\
    return ret;\
}\
catch (std::exception const &e)\
{\
    PLOG_ERROR << e.what();\
    return ret;\
}\
catch (...)\
{\
    PLOG_ERROR << "Unknown exception";\
}
