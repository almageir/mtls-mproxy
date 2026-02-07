#ifndef MTLS_MPROXY_FWD_STREAM_MANAGER_H
#define MTLS_MPROXY_FWD_STREAM_MANAGER_H

#include "transport/stream_manager.h"
#include "fwd_session.h"

#include <asynclog/logger_factory.h>

#include <string>

namespace mtls_mproxy
{
    class FwdStreamManager final
        : public StreamManager
    {
    public:
        explicit FwdStreamManager(const asynclog::LoggerFactory& log_factory, std::string host, std::string port);
        ~FwdStreamManager() override = default;

        FwdStreamManager(const FwdStreamManager& other) = delete;
        FwdStreamManager& operator=(const FwdStreamManager& other) = delete;

        void stop(stream_ptr stream) override;
        void stop(int id) override;
        void on_close(stream_ptr stream) override;

        void on_accept(ServerStreamPtr stream) override;
        void on_read(IoBuffer event, ServerStreamPtr stream) override;
        void on_write(ServerStreamPtr stream) override;
        void on_error(net::error_code ec, ServerStreamPtr stream) override;
        void read_server(int id) override;
        void write_server(int id, IoBuffer event) override;
        void on_server_ready(ServerStreamPtr stream) override;

        void on_connect(IoBuffer event, ClientStreamPtr stream) override;
        void on_read(IoBuffer event, ClientStreamPtr stream) override;
        void on_write(ClientStreamPtr stream) override;
        void on_error(net::error_code ec, ClientStreamPtr stream) override;
        void read_client(int id) override;
        void write_client(int id, IoBuffer event) override;
        void connect(int id, std::string host, std::string service) override;

        std::vector<std::uint8_t> udp_associate(int id) override;

    private:
        struct FwdPair {
            int id;
            ServerStreamPtr server;
            ClientStreamPtr client;
            FwdSession session;
        };

        std::unordered_map<int, FwdPair> sessions_;
        asynclog::LoggerFactory logger_factory_;
        asynclog::ScopedLogger logger_;
        std::string host_;
        std::string port_;
    };
}

#endif // MTLS_MPROXY_FWD_STREAM_MANAGER_H
