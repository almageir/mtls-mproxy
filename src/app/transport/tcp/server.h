#ifndef MTLS_MPROXY_TRANSPORT_SERVER_H
#define MTLS_MPROXY_TRANSPORT_SERVER_H

#include "transport/stream_manager.h"

#include <asio.hpp>

#include <asynclog/logger_factory.h>

namespace mtls_mproxy
{
    using tcp = asio::ip::tcp;
    namespace net = asio;

    class server {
    public:
        explicit server(const std::string& port, StreamManagerPtr proxy_backend, asynclog::LoggerFactory logger_factory);
        virtual ~server();

        server(const server& other) = delete;
        server& operator=(const server& other) = delete;

        void run();
    private:
        net::io_context ctx_;
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

#endif // MTLS_MPROXY_TRANSPORT_SERVER_H
