#ifndef MTLS_MPROXY_HTTP_STREAM_MANAGER_H
#define MTLS_MPROXY_HTTP_STREAM_MANAGER_H

#include "transport/stream_manager.h"
#include "http_session.h"

#include <asynclog/logger_factory.h>

namespace mtls_mproxy
{
    class HttpStreamManager final
        : public StreamManager
    {
    public:
        explicit HttpStreamManager(asynclog::LoggerFactory log_factory);
        ~HttpStreamManager() override = default;

        HttpStreamManager(const HttpStreamManager& other) = delete;
        HttpStreamManager& operator=(const HttpStreamManager& other) = delete;

        void stop(stream_ptr stream) override;
        void stop(int id) override;
        void on_close(stream_ptr stream) override;

        void on_accept(ServerStreamPtr stream) override;
        void on_read(IoBuffer event, ServerStreamPtr stream) override;
        void on_write(IoBuffer event, ServerStreamPtr stream) override;
        void on_error(net::error_code ec, ServerStreamPtr stream) override;
        void read_server(int id) override;
        void write_server(int id, IoBuffer event) override;

        void on_connect(IoBuffer event, ClientStreamPtr stream) override;
        void on_read(IoBuffer event, ClientStreamPtr stream) override;
        void on_write(IoBuffer event, ClientStreamPtr stream) override;
        void on_error(net::error_code ec, ClientStreamPtr stream) override;
        void read_client(int id) override;
        void write_client(int id, IoBuffer event) override;
        void connect(int id, std::string host, std::string service) override;

        std::vector<std::uint8_t> udp_associate(int id) override;

    private:
        struct HttpPair {
            int id;
            ServerStreamPtr Server;
            ClientStreamPtr client;
            HttpSession session;
        };

        std::unordered_map<int, HttpPair> sessions_;
        asynclog::LoggerFactory logger_factory_;
        asynclog::ScopedLogger logger_;
    };

    using http_stream_manager_ptr = std::shared_ptr<HttpStreamManager>;
}

#endif // MTLS_MPROXY_HTTP_STREAM_MANAGER_H
