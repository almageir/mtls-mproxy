#include "tls_server_stream.h"
#include "transport/stream_manager.h"

#include "auxiliary/helpers.h"

#include <asio/write.hpp>

namespace
{
    namespace net = asio;
    using ssl_socket = net::ssl::stream<net::ip::tcp::socket>;

    std::string ep_to_str(const ssl_socket& sock)
    {
        if (!sock.lowest_layer().is_open())
            return "socket not opened";

        net::error_code ec;
        const auto rep = sock.lowest_layer().remote_endpoint(ec);
        if (ec)
            return { "remote_endpoint failed: " + ec.message() };

        return aux::to_string(rep);
    }
}

namespace mtls_mproxy
{
    TlsServerStream::TlsServerStream(const StreamManagerPtr& ptr,
                                     int id,
                                     ssl_socket&& socket,
                                     const asynclog::LoggerFactory& log_factory)
        : ServerStream{ptr, id}
        , socket_{std::move(socket)}
        , logger_{log_factory.create("tls_server_stream")}
        , read_buffer_{}
        , write_buffer_{}
    {
    }

    TlsServerStream::~TlsServerStream()
    {
        logger_.debug(std::format("[{}] tcp Server stream closed", id()));
    }

    net::any_io_executor TlsServerStream::executor() { return socket_.get_executor(); }

    void TlsServerStream::do_start()
    {
        logger_.debug(std::format("[{}] incoming connection from client: [{}]", id(), ep_to_str(socket_)));
        do_handshake();
    }

    void TlsServerStream::do_stop()
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

    void TlsServerStream::do_handshake()
    {
        socket_.async_handshake(
            net::ssl::stream_base::server,
            [this, self{shared_from_this()}](const net::error_code& ec) {
                if (!ec) {
                    do_read();
                }
                else {
                    handle_error(ec);
                }
            });
    }

    void TlsServerStream::do_write(IoBuffer event)
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

    std::vector<std::uint8_t> TlsServerStream::do_udp_associate()
    {
        return {};
    }

    void TlsServerStream::do_read()
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

    void TlsServerStream::handle_error(const net::error_code& ec)
    {
        manager()->on_error(ec, shared_from_this());
    }
}
