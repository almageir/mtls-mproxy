#include "fwd_session.h"
#include "fwd_stream_manager.h"

#include <utility>

namespace mtls_mproxy
{
    FwdSession::FwdSession(int id, StreamManagerPtr mgr, asynclog::LoggerFactory logger_factory)
        : context_{id}, manager_{std::move(mgr)}
        , logger_{logger_factory.create("http_session")}
    {
        state_ = FwdWaitConnection::instance();
    }

    void FwdSession::change_state(std::unique_ptr<FwdState> state)
    {
        state_ = std::move(state);
    }

    void FwdSession::handle_server_read(IoBuffer& event)
    {
        state_->handle_server_read(*this, event);
    }

    void FwdSession::handle_client_read(IoBuffer& event)
    {
        state_->handle_client_read(*this, event);
    }

    void FwdSession::handle_server_write(IoBuffer& event)
    {
        state_->handle_server_write(*this, event);
    }

    void FwdSession::handle_client_write(IoBuffer& event)
    {
        state_->handle_client_write(*this, event);
    }

    void FwdSession::handle_client_connect(IoBuffer& event)
    {
        state_->handle_client_connect(*this, event);
    }

    void FwdSession::handle_on_accept()
    {
        state_->handle_on_accept(*this);
    }

    void FwdSession::handle_server_error(net::error_code ec)
    {
        state_->handle_server_error(*this, ec);
    }

    void FwdSession::handle_client_error(net::error_code ec)
    {
        state_->handle_client_error(*this, ec);
    }

    void FwdSession::update_bytes_sent_to_remote(std::size_t count)
    {
        context().transferred_bytes_to_remote += count;
    }

    void FwdSession::update_bytes_sent_to_local(std::size_t count)
    {
        context().transferred_bytes_to_local += count;
    }

    StreamManagerPtr FwdSession::manager()
    {
        return manager_;
    }

    void FwdSession::connect()
    {
        manager()->connect(id(), std::string{host()}, std::string{service()});
    }

    void FwdSession::stop()
    {
        manager()->stop(id());
    }

    void FwdSession::read_from_server()
    {
        manager()->read_server(id());
    }

    void FwdSession::read_from_client()
    {
        manager()->read_client(id());
    }

    void FwdSession::write_to_client(IoBuffer buffer)
    {
        manager()->write_client(id(), std::move(buffer));
    }

    void FwdSession::write_to_server(IoBuffer buffer)
    {
        manager()->write_server(id(), std::move(buffer));
    }
}