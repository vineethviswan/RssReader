cmake_minimum_required(VERSION 3.28)
project(RssReader LANGUAGES CXX)

include(FetchContent)
include(CTest)

# Fetch GoogleTest
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG v1.15.2
)
# Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(rss src/main.cpp)
target_include_directories(rss PRIVATE ${PROJECT_SOURCE_DIR}/inc)

function(configure_project_target target)
    target_compile_features(${target} PRIVATE cxx_std_23)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive-)
    else()
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endif()
endfunction()

configure_project_target(rss)

if(BUILD_TESTING)
    add_executable(rss_tests tests/LoggerTests.cpp)
    target_include_directories(rss_tests PRIVATE ${PROJECT_SOURCE_DIR}/inc)
    target_link_libraries(rss_tests PRIVATE GTest::gtest GTest::gtest_main)
    configure_project_target(rss_tests)
    add_test(NAME rss_tests COMMAND rss_tests)
endif()

