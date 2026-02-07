#ifndef MTLS_MPROXY_HTTP_STATE_H
#define MTLS_MPROXY_HTTP_STATE_H

#include "transport/io_buffer.h"

#include <asio/error_code.hpp>

#include <memory>

namespace mtls_mproxy
{
    namespace net = asio;

    class HttpSession;

    class HttpState
    {
    public:
        virtual ~HttpState() = default;
        virtual void handle_server_read(HttpSession& session, IoBuffer event);
        virtual void handle_client_read(HttpSession& session, IoBuffer event);
        virtual void handle_on_accept(HttpSession& session);
        virtual void handle_client_connect(HttpSession& session, IoBuffer event);
        virtual void handle_server_write(HttpSession& session);
        virtual void handle_client_write(HttpSession& session);
        virtual void handle_server_error(HttpSession& session, net::error_code ec);
        virtual void handle_client_error(HttpSession& session, net::error_code ec);
    };

    class HttpWaitRequest final : public HttpState
    {
    public:
        static auto instance() { return std::make_unique<HttpWaitRequest>(); };
        void handle_on_accept(HttpSession& session) override;
        void handle_server_read(HttpSession& session, IoBuffer event) override;
    };

    class HttpConnectionEstablished final : public HttpState
    {
    public:
        static auto instance() { return std::make_unique<HttpConnectionEstablished>(); }
        void handle_client_connect(HttpSession& session, IoBuffer event) override;
    };

    class HttpReadyTransferData final : public HttpState
    {
    public:
        static auto instance() { return std::make_unique<HttpReadyTransferData>(); }
        void handle_client_write(HttpSession& session) override;
        void handle_server_write(HttpSession& session) override;
    };

    class HttpDataTransferMode final : public HttpState
    {
    public:
        static auto instance() { return std::make_unique<HttpDataTransferMode>(); }
        void handle_server_read(HttpSession& session, IoBuffer event) override;
        void handle_server_write(HttpSession& session) override;
        void handle_client_read(HttpSession& session, IoBuffer event) override;
        void handle_client_write(HttpSession& session) override;
    };
}

#endif // MTLS_MPROXY_HTTP_STATE_H
