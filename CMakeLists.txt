cmake_minimum_required(VERSION 4.1)

project(mtls-mproxy)

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
)

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

