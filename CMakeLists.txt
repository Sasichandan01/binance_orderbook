cmake_minimum_required(VERSION 3.16)
project(orderbook)


set(CMAKE_CXX_STANDARD 17)


find_package(OpenSSL REQUIRED)


include(FetchContent)


FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.15.0
)
FetchContent_MakeAvailable(spdlog)


add_executable(orderbook
    src/main.cpp
    src/order_book.cpp
    src/snapshot_manager.cpp
    src/websocket_client.cpp
    src/async_logger.cpp  
    src/sync_engine.cpp  
    src/metrics_collector.cpp   
    src/order_manager.cpp
    src/execution_engine.cpp
    src/api_server.cpp  
    src/execution_logger.cpp 
)


target_include_directories(orderbook PRIVATE
    include
    ${OPENSSL_INCLUDE_DIR}
    /opt/homebrew/include
)


target_compile_definitions(orderbook PRIVATE
    CPPHTTPLIB_OPENSSL_SUPPORT
)


target_link_libraries(orderbook PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    spdlog::spdlog
    "-framework CoreFoundation"
    "-framework Security"
    pthread                      # <-- ADDED: Required for std::thread
)