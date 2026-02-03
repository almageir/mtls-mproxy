#include "socks_session.h"
#include "socks_stream_manager.h"

#include <utility>

namespace mtls_mproxy
{
    SocksSession::SocksSession(int id, StreamManagerPtr mgr, const asynclog::LoggerFactory& logger_factory)
        : context_{id}, manager_{std::move(mgr)}, logger_{logger_factory.create("socks5_session")}
    {
        state_ = SocksWaitConnection::instance();
    }

    void SocksSession::change_state(std::unique_ptr<SocksState> state)
    {
        state_ = std::move(state);
    }

    void SocksSession::handle_server_read(IoBuffer event)
    {
        state_->handle_server_read(*this, std::move(event));
    }

    void SocksSession::handle_client_read(IoBuffer event)
    {
        state_->handle_client_read(*this, std::move(event));
    }

    void SocksSession::handle_server_write(IoBuffer event)
    {
        state_->handle_server_write(*this, std::move(event));
    }

    void SocksSession::handle_client_write(IoBuffer event)
    {
        state_->handle_client_write(*this, std::move(event));
    }

    void SocksSession::handle_client_connect(IoBuffer event)
    {
        state_->handle_client_connect(*this, std::move(event));
    }

    void SocksSession::handle_on_accept()
    {
        state_->handle_on_accept(*this);
    }

    void SocksSession::handle_server_error(net::error_code ec)
    {
        state_->handle_server_error(*this, ec);
    }

    void SocksSession::handle_client_error(net::error_code ec)
    {
        state_->handle_client_error(*this, ec);
    }

    void SocksSession::update_bytes_sent_to_remote(std::size_t count)
    {
        context().transferred_bytes_to_remote += count;
    }

    void SocksSession::update_bytes_sent_to_local(std::size_t count)
    {
        context().transferred_bytes_to_local += count;
    }

    StreamManagerPtr SocksSession::manager()
    {
        return manager_;
    }

    asynclog::ScopedLogger& SocksSession::logger()
    {
        return logger_;
    }

    void SocksSession::connect()
    {
        manager()->connect(id(), std::string{host()}, std::string{service()});
    }

    void SocksSession::stop()
    {
        manager()->stop(id());
    }

    void SocksSession::read_from_server()
    {
        manager()->read_server(id());
    }

    void SocksSession::read_from_client()
    {
        manager()->read_client(id());
    }

    std::vector<std::uint8_t> SocksSession::udp_associate()
    {
        return manager()->udp_associate(id());
    }

    void SocksSession::write_to_client(IoBuffer buffer)
    {
        manager()->write_client(id(), std::move(buffer));
    }

    void SocksSession::write_to_server(IoBuffer buffer)
    {
        manager()->write_server(id(), std::move(buffer));
    }
}