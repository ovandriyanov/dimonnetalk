set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(BOOST_INCLUDE_PATH "$ENV{BOOST_INCLUDE_PATH}")
set(BOOST_LIB_PATH "$ENV{BOOST_LIB_PATH}")
set(JSON_INCLUDE_PATH "$ENV{JSON_INCLUDE_PATH}")

include_directories("${PROJECT_SOURCE_DIR}")

if(BOOST_INCLUDE_PATH)
    include_directories("${BOOST_INCLUDE_PATH}")
endif()
if(JSON_INCLUDE_PATH)
    include_directories("${JSON_INCLUDE_PATH}")
endif()

add_library(framework
    inventory.cpp
    config.cpp
    longpoll_update_source.cpp
    update_source_switch.cpp)
