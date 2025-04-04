cmake_minimum_required(VERSION 3.14)
project(procmine VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(PROCMINE_BUILD_TESTS "Build tests" ON)
option(PROCMINE_BUILD_EXAMPLES "Build examples" ON)

find_package(SQLite3 REQUIRED)
find_package(Boost REQUIRED COMPONENTS system filesystem graph)

set(PROCMINE_SOURCES
    src/algorithm.cpp
    src/database.cpp
    src/log.cpp
    src/models.cpp
)

add_library(procmine ${PROCMINE_SOURCES})

target_include_directories(procmine 
    PUBLIC 
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(procmine
    PUBLIC
        SQLite::SQLite3
        Boost::system
        Boost::filesystem
)

if(PROCMINE_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

if(PROCMINE_BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

include(GNUInstallDirs)
install(TARGETS procmine
    EXPORT procmineTargets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}) 