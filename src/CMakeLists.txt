project(dimonnetalk)

set(BOOST_INCLUDE_PATH "$ENV{BOOST_INCLUDE_PATH}")
set(BOOST_LIB_PATH "$ENV{BOOST_LIB_PATH}")

add_subdirectory(framework)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON) #...is required...
set(CMAKE_CXX_EXTENSIONS OFF) #...without compiler extensions like gnu++11

include_directories("${PROJECT_SOURCE_DIR}")

if(BOOST_INCLUDE_PATH)
    include_directories("${BOOST_INCLUDE_PATH}")
endif()

if(BOOST_LIB_PATH)
    link_directories("${BOOST_LIB_PATH}")
endif()

set(cmake_find_library_suffixes ".a")

find_library(BOOST_LOG_SETUP NAMES libboost_log_setup.a boost_log_setup PATHS ${BOOST_LIB_PATH})
find_library(BOOST_LOG NAMES libboost_log.a boost_log PATHS ${BOOST_LIB_PATH})

add_executable(dimonnetalkd main.cpp)
target_link_libraries(dimonnetalkd framework ${BOOST_LOG} ${BOOST_LOG_SETUP} boost_context boost_system boost_thread pthread)
