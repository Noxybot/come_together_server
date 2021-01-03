#include "Server.h"
#include "tests/tests.h"

#include <plog/Initializers/RollingFileInitializer.h>
#include <plog/Appenders/ColorConsoleAppender.h>

int main() try
{
    static plog::ColorConsoleAppender<plog::TxtFormatter> consoleAppender;
    plog::init(plog::verbose, &consoleAppender);
    Server srv;
    RUN_ALL_TESTS();
    srv.Start();
    return 0;
}
catch (std::exception const &e)
{
    std::cerr << e.what() << std::endl;
    return -1;
}
catch (...)
{
    std::cerr << "Unknown exception" << std::endl;
}
