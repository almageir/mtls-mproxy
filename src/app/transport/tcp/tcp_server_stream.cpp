#include "tcp_server_stream.h"
#include "transport/stream_manager.h"

#include <asio/write.hpp>

namespace
{
    namespace net = asio;
    using tcp = asio::ip::tcp;

    std::string ep_to_str(const tcp::socket& sock)
    {
        if (!sock.is_open())
            return "socket not opened";

        net::error_code ec;
        const auto& rep = sock.remote_endpoint(ec);

        if (ec)
            return std::string{"remote_endpoint failed: " + ec.message()};

        net::error_code ignored_ec;
        return { rep.address().to_string() + ":" + std::to_string(rep.port()) };
    }
}

namespace mtls_mproxy
{
    tcp_server_stream::tcp_server_stream(const StreamManagerPtr& ptr,
                                         int id,
                                         tcp::socket&& socket,
                                         const asynclog::LoggerFactory& log_factory)
        : ServerStream{ptr, id}
        , socket_{std::move(socket)}
        , logger_{log_factory.create("tcp_server_stream")}
        , read_buffer_{}
        , write_buffer_{}
    {
    }

    tcp_server_stream::~tcp_server_stream()
    {
        logger_.debug(std::format("[{}] tcp server stream closed", id()));
    }

    net::any_io_executor tcp_server_stream::executor() { return socket_.get_executor(); }

    tcp::socket& tcp_server_stream::socket() { return socket_; }

    void tcp_server_stream::do_start()
    {
        logger_.debug(std::format("[{}] incoming connection from client: [{}]", id(), ep_to_str(socket_)));
        do_read();
    }

    void tcp_server_stream::do_stop()
    {
        net::error_code ignored_ec;
        socket_.shutdown(tcp::socket::shutdown_both, ignored_ec);
    }

    void tcp_server_stream::do_write(IoBuffer event)
    {
        std::ranges::copy(event, write_buffer_.begin());
        net::async_write(
            socket_, net::buffer(write_buffer_, event.size()),
            [this, self{shared_from_this()}](const net::error_code& ec, size_t) {
            if (!ec) {
                manager()->on_write(std::move(IoBuffer{}), shared_from_this());
            }
            else {
                handle_error(ec);
            }
        });
    }

    void tcp_server_stream::do_read()
    {
        socket_.async_read_some(
            net::buffer(read_buffer_),
            [this, self{shared_from_this()}](const net::error_code& ec, const size_t length) {
            if (!ec) {
                IoBuffer event(read_buffer_.data(), read_buffer_.data() + length);
                manager()->on_read(std::move(event), shared_from_this());
            }
            else {
                handle_error(ec);
            }
        });
    }

    void tcp_server_stream::handle_error(const net::error_code& ec)
    {
        manager()->on_error(ec, shared_from_this());
    }
}