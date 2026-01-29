#include "tls_server_stream.h"
#include "transport/stream_manager.h"

namespace
{
    std::string ep_to_str(const ssl_socket& sock)
    {
        if (!sock.lowest_layer().is_open())
            return "socket not opened";

        net::error_code ec;
        const auto rep = sock.lowest_layer().remote_endpoint(ec);
        if (ec)
            return { "remote_endpoint failed: " + ec.message() };

        return { rep.address().to_string() + ":" + std::to_string(rep.port()) };
    }
}

tls_server_stream::tls_server_stream(const stream_manager_ptr& ptr,
    int id, net::io_context& ctx,
    net::ssl::context& ssl_ctx,
    asynclog::LoggerFactory log_factory)
    : server_stream{ptr, id}
    , ctx_{ctx}
    , ssl_ctx_{ssl_ctx}
    , socket_{ctx_, ssl_ctx_}
    , logger_{log_factory.create("tls_server_stream")}
    , read_buffer_{}
    , write_buffer_{}
{}

tls_server_stream::~tls_server_stream()
{
    logger_.debug(std::format("[{}] tcp server stream closed", id()));
}

net::io_context& tls_server_stream::context() { return ctx_; }

tcp::socket& tls_server_stream::socket()
{
    return socket_.next_layer();
}

void tls_server_stream::do_start()
{
    logger_.debug(std::format("[{}] incoming connection from client: [{}]", id(), ep_to_str(socket_)));
    do_handshake();
}

void tls_server_stream::do_stop()
{
    if (!socket_.lowest_layer().is_open())
        return;

    net::error_code ignored_ec;
    socket_.lowest_layer().cancel(ignored_ec);

    socket_.async_shutdown(
        [this, self{shared_from_this()}](const net::error_code& ec) {
            if (ec)
                handle_error(ec);
            socket_.lowest_layer().close();
        });

    net::async_write(
        socket_, net::null_buffers{},
        [this, self{shared_from_this()}](const net::error_code& ec, std::size_t trans_bytes) {
            if (ec)
                handle_error(ec);
            socket_.lowest_layer().close();
        });
}

void tls_server_stream::do_handshake()
{
    socket_.async_handshake(
        net::ssl::stream_base::server,
        [this, self{shared_from_this()}](const net::error_code& ec) {
        if (!ec) {
            do_read();
        } else {
            handle_error(ec);
        }
    });
}

void tls_server_stream::do_write(io_buffer event)
{
    copy(event.begin(), event.end(), write_buffer_.begin());
    net::async_write(
            socket_, net::buffer(write_buffer_, event.size()),
            [this, self{shared_from_this()}](const net::error_code &ec, size_t) {
                if (!ec) {
                    manager()->on_write(std::move(io_buffer{}), shared_from_this());
                } else {
                    handle_error(ec);
                }
            });
}

void tls_server_stream::do_read()
{
    socket_.async_read_some(
            net::buffer(read_buffer_),
            [this, self{shared_from_this()}](const net::error_code &ec, const size_t length) {
                if (!ec) {
                    io_buffer event(read_buffer_.data(), read_buffer_.data() + length);
                    manager()->on_read(std::move(event), shared_from_this());
                } else {
                    handle_error(ec);
                }
            });
}

void tls_server_stream::handle_error(const net::error_code& ec)
{
    manager()->on_error(ec, shared_from_this());
}

