#include "Logger.hpp"

#include <gtest/gtest.h>
#include <iostream>
#include <sstream>

TEST(LoggerTest, MinLevelFiltersDebugMessages)
{
    Logger::SetMinLevel(Logger::Level::INFO);

    std::ostringstream output;
    auto* originalBuffer = std::cout.rdbuf(output.rdbuf());
    {
        Logger::Log(Logger::Level::DEBUG, "filtered message");
        Logger::Log(Logger::Level::INFO, "visible message");
    }
    std::cout.rdbuf(originalBuffer);

    std::string loggedOutput = output.str();
    EXPECT_TRUE(loggedOutput.find("filtered message") == std::string::npos);
    EXPECT_TRUE(loggedOutput.find("visible message") != std::string::npos);
}

TEST(LoggerTest, MinLevelAllowsAllAtDebug)
{
    Logger::SetMinLevel(Logger::Level::DEBUG);

    std::ostringstream output;
    auto* originalBuffer = std::cout.rdbuf(output.rdbuf());
    {
        Logger::Log(Logger::Level::DEBUG, "debug message");
        Logger::Log(Logger::Level::INFO, "info message");
    }
    std::cout.rdbuf(originalBuffer);

    std::string loggedOutput = output.str();
    EXPECT_TRUE(loggedOutput.find("debug message") != std::string::npos);
    EXPECT_TRUE(loggedOutput.find("info message") != std::string::npos);
}

TEST(LoggerTest, MinLevelFiltersUpToWarning)
{
    Logger::SetMinLevel(Logger::Level::WARNING);

    std::ostringstream output;
    std::ostringstream errorOutput;
    auto* originalOut = std::cout.rdbuf(output.rdbuf());
    auto* originalErr = std::cerr.rdbuf(errorOutput.rdbuf());
    {
        Logger::Log(Logger::Level::DEBUG, "debug message");
        Logger::Log(Logger::Level::INFO, "info message");
        Logger::Log(Logger::Level::WARNING, "warning message");
    }
    std::cout.rdbuf(originalOut);
    std::cerr.rdbuf(originalErr);

    std::string out = output.str();
    std::string err = errorOutput.str();
    EXPECT_TRUE(out.find("debug message") == std::string::npos);
    EXPECT_TRUE(out.find("info message") == std::string::npos);
    EXPECT_TRUE(err.find("warning message") != std::string::npos);
}

TEST(LoggerTest, GetMinLevelReturnsCurrentLevel)
{
    Logger::SetMinLevel(Logger::Level::DEBUG);
    EXPECT_EQ(Logger::GetMinLevel(), Logger::Level::DEBUG);

    Logger::SetMinLevel(Logger::Level::ERROR);
    EXPECT_EQ(Logger::GetMinLevel(), Logger::Level::ERROR);
}
