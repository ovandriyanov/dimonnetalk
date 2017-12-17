set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(BOOST_INCLUDE_PATH "$ENV{BOOST_INCLUDE_PATH}")
set(BOOST_LIB_PATH "$ENV{BOOST_LIB_PATH}")
set(JSON_INCLUDE_PATH "$ENV{JSON_INCLUDE_PATH}")
set(LUA_INCLUDE_PATH "$ENV{LUA_INCLUDE_PATH}")

include_directories("${PROJECT_SOURCE_DIR}")

if(BOOST_INCLUDE_PATH)
    include_directories("${BOOST_INCLUDE_PATH}")
endif()
if(JSON_INCLUDE_PATH)
    include_directories("${JSON_INCLUDE_PATH}")
endif()
if(OPENSSL_INCLUDE_PATH)
    include_directories("${OPENSSL_INCLUDE_PATH}")
endif()
if(LUA_INCLUDE_PATH)
    include_directories("${LUA_INCLUDE_PATH}")
endif()

add_library(framework
    api_server.cpp
    bot_store.cpp
    config.cpp
    inventory.cpp
    longpoll_update_source.cpp
    lua/bot.cpp
    lua/io.cpp
    lua/io/timer.cpp
    lua/log.cpp
    server_connector.cpp
    update_source_switch.cpp
)
