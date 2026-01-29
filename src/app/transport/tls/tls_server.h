#ifndef TLS_SERVER_H
#define TLS_SERVER_H

#include "transport/stream_manager.h"

#include <asio.hpp>
#include <asio/ssl.hpp>

#include <filesystem>

#include "asynclog/logger_factory.h"
#include "src/detail/context_bound_log_backend.h"

namespace net = asio;
using tcp = net::ip::tcp;
namespace fs = std::filesystem;

class tls_server
{
public:
    struct tls_options {
        std::string private_key;
        std::string server_cert;
        std::string ca_cert;
    };

    explicit tls_server(const std::string& port, tls_options settings, stream_manager_ptr proxy_backend, asynclog::LoggerFactory log_factory);
    virtual ~tls_server();

    tls_server(const tls_server& other) = delete;
    tls_server& operator=(const tls_server& other) = delete;

    void run();
private:
    net::io_context ctx_;
    net::ssl::context ssl_ctx_;
    net::signal_set signals_;
    tcp::acceptor acceptor_;
    stream_manager_ptr stream_manager_;
    asynclog::LoggerFactory logger_factory_;
    asynclog::ScopedLogger logger_;
    int stream_id_;

    void configure_signals();
    void async_wait_signals();

    void start_accept();
};


#endif //TLS_SERVER_H