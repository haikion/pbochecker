cmake_minimum_required(VERSION 3.16)
project(pbocheckertest)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenSSL REQUIRED)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)

include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.16.0
)

# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

include(GoogleTest)
enable_testing()

add_executable(
  pbocheckertest
  ../lib/pbochecker.h ../lib/pbochecker.cpp
  ../lib/pbocheckerprinterqt.h ../lib/pbocheckerprinterqt.cpp
  pbocheckertest.cpp
)

target_link_libraries(
  pbocheckertest
  GTest::gtest_main
  GTest::gmock
  OpenSSL::SSL
  OpenSSL::Crypto
  Qt6::Core
)

file(GLOB_RECURSE PBOCHECKER_TEST_RESOURCES testresources/*)
qt_add_resources(pbocheckertest testresources
    PREFIX "/"
    BASE testresources
    FILES ${PBOCHECKER_TEST_RESOURCES}
)

target_include_directories(
  pbocheckertest
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/../lib
)

target_compile_definitions(pbocheckertest PRIVATE UNIT_TEST)

gtest_discover_tests(pbocheckertest)
