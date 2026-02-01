#ifndef MTLS_MPROXY_TRANSPORT_SERVER_H
#define MTLS_MPROXY_TRANSPORT_SERVER_H

#include "transport/stream_manager.h"

#include <asio/io_context.hpp>
#include <asio/ip/tcp.hpp>
#include <asio/signal_set.hpp>

#include <asynclog/logger_factory.h>

namespace mtls_mproxy
{
    using tcp = asio::ip::tcp;
    namespace net = asio;

    class Server {
    public:
        explicit Server(const std::string& port, StreamManagerPtr proxy_backend, asynclog::LoggerFactory logger_factory);
        virtual ~Server();

        Server(const Server& other) = delete;
        Server& operator=(const Server& other) = delete;

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
