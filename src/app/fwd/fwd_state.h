#ifndef MTLS_MPROXY_FWD_STATE_H
#define MTLS_MPROXY_FWD_STATE_H

#include "transport/io_buffer.h"

#include <asio/error_code.hpp>

#include <memory>
#include <format>

namespace mtls_mproxy
{
    namespace net = asio;

    class FwdSession;

    class FwdState
    {
    public:
        virtual ~FwdState() = default;
        virtual void handle_server_read(FwdSession& session, IoBuffer event);
        virtual void handle_client_read(FwdSession& session, IoBuffer event);
        virtual void handle_on_accept(FwdSession& session);
        virtual void handle_client_connect(FwdSession& session, IoBuffer event);
        virtual void handle_server_write(FwdSession& session, IoBuffer event);
        virtual void handle_client_write(FwdSession& session, IoBuffer event);
        virtual void handle_server_error(FwdSession& session, net::error_code ec);
        virtual void handle_client_error(FwdSession& session, net::error_code ec);
    };

    class FwdWaitConnection final : public FwdState
    {
    public:
        static auto instance() { return std::make_unique<FwdWaitConnection>(); };
        void handle_on_accept(FwdSession& session) override;
    };

    class FwdConnectionEstablished final : public FwdState
    {
    public:
        static auto instance() { return std::make_unique<FwdConnectionEstablished>(); }
        void handle_client_connect(FwdSession& session, IoBuffer event) override;
        void handle_server_read(FwdSession& session, IoBuffer event) override;
    };

    class FwdDataTransferMode final : public FwdState
    {
    public:
        static auto instance() { return std::make_unique<FwdDataTransferMode>(); }
        void handle_server_read(FwdSession& session, IoBuffer event) override;
        void handle_server_write(FwdSession& session, IoBuffer event) override;
        void handle_client_read(FwdSession& session, IoBuffer event) override;
        void handle_client_write(FwdSession& session, IoBuffer event) override;
    };
}

#endif // MTLS_MPROXY_FWD_STATE_H
