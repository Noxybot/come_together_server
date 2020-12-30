#pragma once
#include <iostream>
#define CATCH_ALL(ret) catch (pqxx::sql_error const &e)\
{\
    std::cerr\
    << "Database error: " << e.what() << std::endl\
    << "Query was: " << e.query() << std::endl;\
    return ret;\
}\
catch (std::exception const &e)\
{\
    std::cerr << e.what() << std::endl;\
    return ret;\
}\
catch (...)\
{\
    std::cerr << "Unknown exception" << std::endl;\
}
