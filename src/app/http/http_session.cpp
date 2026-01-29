#include "http_session.h"
#include "http_stream_manager.h"

#include <utility>

http_session::http_session(int id, stream_manager_ptr mgr, asynclog::LoggerFactory log_factory)
    : context_{id}, manager_{std::move(mgr)}, log_factory_{log_factory}
    , logger_{log_factory_.create("http_session")}
{
    state_ = http_wait_request::instance();
}

void http_session::change_state(std::unique_ptr<http_state> state)
{
    state_ = std::move(state);
}

void http_session::handle_server_read(io_buffer &event)
{
    state_->handle_server_read(this, event);
}

void http_session::handle_client_read(io_buffer &event)
{
    state_->handle_client_read(this, event);
}

void http_session::handle_server_write(io_buffer &event)
{
    state_->handle_server_write(this, event);
}

void http_session::handle_client_write(io_buffer &event)
{
    state_->handle_client_write(this, event);
}

void http_session::handle_client_connect(io_buffer &event)
{
    state_->handle_client_connect(this, event);
}

void http_session::handle_server_error(net::error_code ec)
{
    state_->handle_server_error(this, ec);
}

void http_session::handle_client_error(net::error_code ec)
{
    state_->handle_client_error(this, ec);
}

void http_session::update_bytes_sent_to_remote(std::size_t count)
{
	context().transferred_bytes_to_remote += count;
}

void http_session::update_bytes_sent_to_local(std::size_t count)
{
	context().transferred_bytes_to_local += count;
}

stream_manager_ptr http_session::manager()
{
    return manager_;
}

void http_session::connect()
{
	manager()->connect(id(), std::string{ host() }, std::string{ service() });
}

void http_session::stop()
{
	manager()->stop(id());
}

void http_session::read_from_server()
{
	manager()->read_server(id());
}

void http_session::read_from_client()
{
	manager()->read_client(id());
}

void http_session::write_to_client(io_buffer buffer)
{
	manager()->write_client(id(), std::move(buffer));
}

void http_session::write_to_server(io_buffer buffer)
{
	manager()->write_server(id(), std::move(buffer));
}

