#ifndef MTLS_MPROXY_HTTP_STREAM_MANAGER_H
#define MTLS_MPROXY_HTTP_STREAM_MANAGER_H

#include "transport/stream_manager.h"
#include "http_session.h"

#include <asynclog/logger_factory.h>

namespace mtls_mproxy
{
    class HttpStreamManager final
        : public StreamManager
        , public std::enable_shared_from_this<HttpStreamManager>
    {
    public:
        explicit HttpStreamManager(const asynclog::LoggerFactory& log_factory);
        ~HttpStreamManager() override = default;

        HttpStreamManager(const HttpStreamManager& other) = delete;
        HttpStreamManager& operator=(const HttpStreamManager& other) = delete;

        void stop(int id) override;

        void on_accept(ServerStreamPtr stream) override;
        void on_read(IoBuffer event, ServerStreamPtr stream) override;
        void on_write(ServerStreamPtr stream) override;
        void on_error(net::error_code ec, ServerStreamPtr stream) override;
        void read_server(int id) override;
        void write_server(int id, IoBuffer event) override;
        void on_server_ready(ServerStreamPtr ptr) override;

        void on_connect(IoBuffer event, ClientStreamPtr stream) override;
        void on_read(IoBuffer event, ClientStreamPtr stream) override;
        void on_write(ClientStreamPtr stream) override;
        void on_error(net::error_code ec, ClientStreamPtr stream) override;
        void read_client(int id) override;
        void write_client(int id, IoBuffer event) override;
        void connect(int id, std::string host, std::string service) override;

        std::vector<std::uint8_t> udp_associate(int id) override;

    private:
        struct HttpPair {
            int id;
            ServerStreamPtr server;
            ClientStreamPtr client;
            HttpSession session;
        };

        std::unordered_map<int, HttpPair> sessions_;
        asynclog::LoggerFactory logger_factory_;
        asynclog::ScopedLogger logger_;
    };
}

#endif // MTLS_MPROXY_HTTP_STREAM_MANAGER_H
