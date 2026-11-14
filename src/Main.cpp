#include "Logger.hpp"

int main()
{
    Logger::InitializeFromEnvironment();
    Logger::Log(Logger::Level::INFO, "RSS Reader!!");

    return 0;
}
