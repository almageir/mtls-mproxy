cmake_minimum_required(VERSION 3.21)

project(mtls-mproxy)

configure_file(${PROJECT_SOURCE_DIR}/config/test-ca.pem ${CMAKE_CURRENT_BINARY_DIR}/test-ca.pem COPYONLY)
configure_file(${PROJECT_SOURCE_DIR}/config/test-server-cert.pem ${CMAKE_CURRENT_BINARY_DIR}/test-server-cert.pem COPYONLY)
configure_file(${PROJECT_SOURCE_DIR}/config/test-server-key.pem ${CMAKE_CURRENT_BINARY_DIR}/test-server-key.pem COPYONLY)

if(WIN32)
    set(OPENSSL_USE_STATIC_LIBS     TRUE)
    set(OPENSSL_MSVC_STATIC_RT      TRUE)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    set(CMAKE_VS_JUST_MY_CODE_DEBUGGING "$<$<CONFIG:Debug>:ON>")
endif()

find_package(OpenSSL REQUIRED)
if(OPENSSL_FOUND)
    message(STATUS "OpenSSL path:           ${OPENSSL_ROOT_DIR}")
    message(STATUS "OpenSSL include path:   ${OPENSSL_INCLUDE_DIR}")
    message(STATUS "OpenSSL libraries path: ${OPENSSL_LIBRARIES}")
else()
    message(FATAL "OpenSSL Not Found")
endif()

include(FetchContent)

FetchContent_Declare(asio
    GIT_REPOSITORY https://github.com/chriskohlhoff/asio.git
    GIT_TAG        asio-1-36-0
)
FetchContent_MakeAvailable(asio)

add_library(asio INTERFACE)
target_include_directories(asio INTERFACE ${asio_SOURCE_DIR}/asio/include)

FetchContent_Declare(cliap
    GIT_REPOSITORY https://github.com/almageir/cliap.git
    GIT_TAG        master
)
option(CLIAP_STANDALONE_BUILD off)
FetchContent_MakeAvailable(cliap)


option(ASIOXX_BUILD_EXAMPLES "Build examples" OFF)
add_subdirectory(libs/asioxx)

add_executable(mtls-mproxy)

target_sources(mtls-mproxy
    PRIVATE
        src/main.cpp

        # Common types
        src/app/auxiliary/helpers.h
        src/app/auxiliary/helpers.cpp
        src/app/transport/io_buffer.h
        src/app/transport/server_stream.h
        src/app/transport/client_stream.h
        src/app/transport/stream_manager.h

        # Outgoing proxy tcp connections support
        src/app/transport/tcp_client_stream.h
        src/app/transport/tcp_client_stream.cpp

        # Outgoing proxy udp connections support
        src/app/transport/udp_client_stream.h
        src/app/transport/udp_client_stream.cpp

        # Incoming plain tcp support
        src/app/transport/tcp/server.h
        src/app/transport/tcp/server.cpp
        src/app/transport/tcp/tcp_server_stream.h
        src/app/transport/tcp/tcp_server_stream.cpp

        # Incoming mTLS support
        src/app/transport/tls/tls_server.h
        src/app/transport/tls/tls_server.cpp
        src/app/transport/tls/tls_server_stream.h
        src/app/transport/tls/tls_server_stream.cpp

        # Http(s) proxy
        src/app/http/http.h
        src/app/http/http.cpp
        src/app/http/http_session.h
        src/app/http/http_session.cpp
        src/app/http/http_state.h
        src/app/http/http_state.cpp
        src/app/http/http_stream_manager.h
        src/app/http/http_stream_manager.cpp

        # Http(s) proxy
        src/app/fwd/fwd_session.h
        src/app/fwd/fwd_session.cpp
        src/app/fwd/fwd_state.h
        src/app/fwd/fwd_state.cpp
        src/app/fwd/fwd_stream_manager.h
        src/app/fwd/fwd_stream_manager.cpp

        # Socks5 proxy
        src/app/socks/socks.h
        src/app/socks/socks.cpp
        src/app/socks/socks_session.h
        src/app/socks/socks_session.cpp
        src/app/socks/socks_state.h
        src/app/socks/socks_state.cpp
        src/app/socks/socks_stream_manager.h
        src/app/socks/socks_stream_manager.cpp
 "src/app/auxiliary/helpers.h")

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src/app)

target_link_libraries(mtls-mproxy
    PRIVATE
    asio
    cliap::cliap
    OpenSSL::SSL
    OpenSSL::Crypto
    asynclog::asynclog
)

target_compile_features(mtls-mproxy PRIVATE cxx_std_20)
if (WIN32)
    target_compile_definitions(mtls-mproxy PRIVATE "_WIN32_WINNT=0x0A00")
endif ()

