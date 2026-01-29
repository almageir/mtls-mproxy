#ifndef MTLS_MPROXY_TRANSPORT_TLS_SERVER_H
#define MTLS_MPROXY_TRANSPORT_TLS_SERVER_H

#include "transport/stream_manager.h"

#include <asynclog/logger_factory.h>

#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/ssl.hpp>
#include <asio/signal_set.hpp>

#include <filesystem>

namespace mtls_mproxy
{
    namespace net = asio;
    using tcp = net::ip::tcp;
    namespace fs = std::filesystem;

    class TlsServer
    {
    public:
        struct TlsOptions {
            std::string private_key;
            std::string server_cert;
            std::string ca_cert;
        };

        explicit TlsServer(const std::string& port,
                           const TlsOptions& settings,
                           StreamManagerPtr proxy_backend,
                           asynclog::LoggerFactory log_factory);
        virtual ~TlsServer();

        TlsServer(const TlsServer& other) = delete;
        TlsServer& operator=(const TlsServer& other) = delete;

        void run();
    private:
        net::io_context ctx_;
        net::ssl::context ssl_ctx_;
        net::signal_set signals_;
        tcp::acceptor acceptor_;
        StreamManagerPtr stream_manager_;
        asynclog::LoggerFactory logger_factory_;
        asynclog::ScopedLogger logger_;
        int stream_id_;

        void configure_signals();
        void async_wait_signals();

        void start_accept();
    };
}

#endif // MTLS_MPROXY_TRANSPORT_TLS_SERVER_H