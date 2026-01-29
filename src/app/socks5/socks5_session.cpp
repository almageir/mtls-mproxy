#include "socks5_session.h"
#include "socks5_stream_manager.h"

#include <utility>

socks5_session::socks5_session(int id, stream_manager_ptr mgr, asynclog::LoggerFactory factory)
    : context_{id}, manager_{std::move(mgr)}, logger_{factory.create("socks5_session")}
{
    state_ = socks5_auth_request::instance();
}

void socks5_session::change_state(std::unique_ptr<socks5_state> state)
{
    state_ = std::move(state);
}

void socks5_session::handle_server_read(io_buffer event)
{
    state_->handle_server_read(this, std::move(event));
}

void socks5_session::handle_client_read(io_buffer event)
{
    state_->handle_client_read(this, std::move(event));
}

void socks5_session::handle_server_write(io_buffer event)
{
    state_->handle_server_write(this, std::move(event));
}

void socks5_session::handle_client_write(io_buffer event)
{
    state_->handle_client_write(this, std::move(event));
}

void socks5_session::handle_client_connect(io_buffer event)
{
    state_->handle_client_connect(this, std::move(event));
}

void socks5_session::handle_server_error(net::error_code ec)
{
    state_->handle_server_error(this, ec);
}

void socks5_session::handle_client_error(net::error_code ec)
{
    state_->handle_client_error(this, ec);
}

void socks5_session::update_bytes_sent_to_remote(std::size_t count)
{
    context().transferred_bytes_to_remote += count;
}

void socks5_session::update_bytes_sent_to_local(std::size_t count)
{
    context().transferred_bytes_to_local += count;
}

stream_manager_ptr socks5_session::manager()
{
    return manager_;
}

asynclog::ScopedLogger& socks5_session::logger()
{
    return logger_;
}

void socks5_session::connect()
{
	manager()->connect(id(), std::string{ host() }, std::string{ service() });
}

void socks5_session::stop()
{
    manager()->stop(id());
}

void socks5_session::read_from_server()
{
    manager()->read_server(id());
}

void socks5_session::read_from_client()
{
    manager()->read_client(id());
}

void socks5_session::write_to_client(io_buffer buffer)
{
    manager()->write_client(id(), std::move(buffer));
}

void socks5_session::write_to_server(io_buffer buffer)
{
	manager()->write_server(id(), std::move(buffer));
}
