#ifndef MTLS_MPROXY_SOCKS_STATE_H
#define MTLS_MPROXY_SOCKS_STATE_H

#include "transport/io_buffer.h"

#include <asio/error_code.hpp>

#include <memory>
#include <format>

namespace net = asio;

namespace mtls_mproxy
{
    class SocksSession;

    class SocksState
    {
    public:
        virtual ~SocksState() = default;
        virtual void handle_server_read(SocksSession& session, IoBuffer event);
        virtual void handle_client_read(SocksSession& session, IoBuffer event);
        virtual void handle_client_connect(SocksSession& session, IoBuffer event);
        virtual void handle_server_write(SocksSession& session, IoBuffer event);
        virtual void handle_client_write(SocksSession& session, IoBuffer event);
        virtual void handle_server_error(SocksSession& session, net::error_code ec);
        virtual void handle_client_error(SocksSession& session, net::error_code ec);
    };

    class SocksAuthRequest final : public SocksState
    {
    public:
        static auto instance() { return std::make_unique<SocksAuthRequest>(); }
        void handle_server_read(SocksSession& session, IoBuffer event) override;
    };

    class SocksConnectionRequest final : public SocksState
    {
    public:
        static auto instance() { return std::make_unique<SocksConnectionRequest>(); }
        void handle_server_read(SocksSession& session, IoBuffer event) override;
        void handle_server_write(SocksSession& session, IoBuffer event) override;
    };

    class SocksConnectionEstablished final : public SocksState
    {
    public:
        static auto instance() { return std::make_unique<SocksConnectionEstablished>(); }
        void handle_client_connect(SocksSession& session, IoBuffer event) override;
        void handle_client_error(SocksSession& session, net::error_code ec) override;
        void handle_server_write(SocksSession& session, IoBuffer event) override;
    };

    class SocksReadyTransferData final : public SocksState
    {
    public:
        static auto instance() { return std::make_unique<SocksReadyTransferData>(); }
        void handle_server_write(SocksSession& session, IoBuffer event) override;
    };

    class SocksReadyUdpTransferData final : public SocksState
    {
    public:
        static auto instance() { return std::make_unique<SocksReadyUdpTransferData>(); }
        void handle_server_write(SocksSession& session, IoBuffer event) override;
    };

    class SocksDataTransferMode final : public SocksState
    {
    public:
        static auto instance() { return std::make_unique<SocksDataTransferMode>(); }
        void handle_server_read(SocksSession& session, IoBuffer event) override;
        void handle_server_write(SocksSession& session, IoBuffer event) override;
        void handle_client_read(SocksSession& session, IoBuffer event) override;
        void handle_client_write(SocksSession& session, IoBuffer event) override;
    };

    class SocksDataUdpTransferMode final : public SocksState
    {
    public:
        static auto instance() { return std::make_unique<SocksDataUdpTransferMode>(); }
        void handle_server_read(SocksSession& session, IoBuffer event) override;
        void handle_server_write(SocksSession& session, IoBuffer event) override;
        void handle_client_read(SocksSession& session, IoBuffer event) override;
        void handle_client_write(SocksSession& session, IoBuffer event) override;
    };
}
#endif // MTLS_MPROXY_SOCKS_STATE_H
