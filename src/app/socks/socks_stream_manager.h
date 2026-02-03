#ifndef MTLS_MPROXY_SOCKS_STREAM_MANAGER_H
#define MTLS_MPROXY_SOCKS_STREAM_MANAGER_H

#include "transport/stream_manager.h"
#include "socks_session.h"

#include <asynclog/logger_factory.h>

namespace mtls_mproxy
{
    class SocksStreamManager final
        : public StreamManager
    {
    public:
        explicit SocksStreamManager(asynclog::LoggerFactory log_factory, bool udp_enabled = false);
        ~SocksStreamManager() override = default;

        SocksStreamManager(const SocksStreamManager& other) = delete;
        SocksStreamManager& operator=(const SocksStreamManager& other) = delete;

        void stop(stream_ptr stream) override;
        void stop(int id) override;
        void on_close(stream_ptr stream) override;

        void on_accept(ServerStreamPtr stream) override;
        void on_read(IoBuffer buffer, ServerStreamPtr stream) override;
        void on_write(IoBuffer buffer, ServerStreamPtr stream) override;
        void on_error(net::error_code ec, ServerStreamPtr stream) override;
        void read_server(int id) override;
        void write_server(int id, IoBuffer buffer) override;
        void on_server_ready(ServerStreamPtr ptr) override;

        void on_connect(IoBuffer buffer, ClientStreamPtr stream) override;
        void on_read(IoBuffer buffer, ClientStreamPtr stream) override;
        void on_write(IoBuffer buffer, ClientStreamPtr stream) override;
        void on_error(net::error_code ec, ClientStreamPtr stream) override;
        void read_client(int id) override;
        void write_client(int id, IoBuffer buffer) override;
        void connect(int id, std::string host, std::string service) override;

        std::vector<std::uint8_t> udp_associate(int id) override;

    private:
        struct SocksPair {
            int id;
            ServerStreamPtr server;
            ClientStreamPtr client;
            SocksSession session;
        };

        std::unordered_map<int, SocksPair> sessions_;
        asynclog::LoggerFactory logger_factory_;
        asynclog::ScopedLogger logger_;
        bool is_udp_associate_mode_enabled_{false};
    };
}

#endif // MTLS_MPROXY_SOCKS_STREAM_MANAGER_H
