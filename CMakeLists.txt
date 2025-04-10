cmake_minimum_required(VERSION 3.16)

project(pbochecker LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(OpenSSL REQUIRED)
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)

if(WIN32)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /SUBSYSTEM:CONSOLE")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>" CACHE INTERNAL "")
endif()

add_subdirectory(lib)
if (UNIX)
add_subdirectory(tests)
endif()
add_executable(pbochecker
  main.cpp
  ${PBOCHECKER_LIB_SOURCES}
)
target_include_directories(pbochecker PRIVATE lib)

target_link_libraries(pbochecker
  OpenSSL::SSL
  OpenSSL::Crypto
  Qt${QT_VERSION_MAJOR}::Core
)

if (WIN32)
    target_link_libraries(pbochecker Crypt32)
endif()

include(GNUInstallDirs)
install(TARGETS pbochecker
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

