set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(BOOST_INCLUDE_PATH "$ENV{BOOST_INCLUDE_PATH}")
set(BOOST_LIB_PATH "$ENV{BOOST_LIB_PATH}")

include_directories("${PROJECT_SOURCE_DIR}")

if(BOOST_INCLUDE_PATH)
    include_directories("${BOOST_INCLUDE_PATH}")
endif()

add_library(util
    asio.cpp
    file_chunk_range.cpp
)
