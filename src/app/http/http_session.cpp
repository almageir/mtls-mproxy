#include "http_session.h"
#include "http_stream_manager.h"

#include <utility>

namespace mtls_mproxy
{
    HttpSession::HttpSession(int id, StreamManagerPtr mgr, const asynclog::LoggerFactory& logger_factory)
        : context_{id}, manager_{std::move(mgr)}
        , logger_{logger_factory.create("http_session")}
    {
        state_ = HttpWaitRequest::instance();
    }

    void HttpSession::change_state(std::unique_ptr<HttpState> state)
    {
        state_ = std::move(state);
    }

    void HttpSession::handle_server_read(IoBuffer& event)
    {
        state_->handle_server_read(*this, event);
    }

    void HttpSession::handle_client_read(IoBuffer& event)
    {
        state_->handle_client_read(*this, event);
    }

    void HttpSession::handle_server_write()
    {
        state_->handle_server_write(*this);
    }

    void HttpSession::handle_client_write()
    {
        state_->handle_client_write(*this);
    }

    void HttpSession::handle_client_connect(IoBuffer& event)
    {
        state_->handle_client_connect(*this, event);
    }

    void HttpSession::handle_on_accept()
    {
        state_->handle_on_accept(*this);
    }

    void HttpSession::handle_server_error(net::error_code ec)
    {
        state_->handle_server_error(*this, ec);
    }

    void HttpSession::handle_client_error(net::error_code ec)
    {
        state_->handle_client_error(*this, ec);
    }

    void HttpSession::update_bytes_sent_to_remote(std::size_t count)
    {
        context().transferred_bytes_to_remote += count;
    }

    void HttpSession::update_bytes_sent_to_local(std::size_t count)
    {
        context().transferred_bytes_to_local += count;
    }

    StreamManagerPtr HttpSession::manager()
    {
        return manager_;
    }

    void HttpSession::connect()
    {
        manager()->connect(id(), std::string{host()}, std::string{service()});
    }

    void HttpSession::stop()
    {
        manager()->stop(id());
    }

    void HttpSession::read_from_server()
    {
        manager()->read_server(id());
    }

    void HttpSession::read_from_client()
    {
        manager()->read_client(id());
    }

    void HttpSession::write_to_client(IoBuffer buffer)
    {
        manager()->write_client(id(), std::move(buffer));
    }

    void HttpSession::write_to_server(IoBuffer buffer)
    {
        manager()->write_server(id(), std::move(buffer));
    }
}