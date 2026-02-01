#include "tcp_server_stream.h"
#include "transport/stream_manager.h"
#include "auxiliary/helpers.h"

#include <asio/write.hpp>

namespace
{
    namespace net = asio;
    using tcp = asio::ip::tcp;
    using udp = asio::ip::udp;

    constexpr std::uint8_t ATYPE_IPv4 = 0x01;
    constexpr std::uint8_t ATYPE_DOMAIN = 0x03;
    constexpr std::uint8_t ATYPE_IPv6 = 0x04;
    constexpr std::size_t SOCKS5_UDP_HEADER_SIZE = 0x04;

    std::string ep_to_str(const tcp::socket& sock)
    {
        if (!sock.is_open())
            return "socket not opened";

        net::error_code ec;
        const auto& rep = sock.remote_endpoint(ec);

        if (ec)
            return std::string{"remote_endpoint failed: " + ec.message()};

        return aux::to_string(rep);
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
        , udp_read_buffer_{}
        , udp_write_buffer_{}
    {
    }

    tcp_server_stream::~tcp_server_stream()
    {
        logger_.debug(std::format("[{}] tcp Server stream closed", id()));
    }

    net::any_io_executor tcp_server_stream::executor() { return socket_.get_executor(); }

    void tcp_server_stream::do_start()
    {
        logger_.debug(std::format("[{}] incoming connection from client: [{}]", id(), ep_to_str(socket_)));
        do_read();
    }

    void tcp_server_stream::do_stop()
    {
        net::error_code ignored_ec;
        socket_.shutdown(tcp::socket::shutdown_both, ignored_ec);
        if (udp_socket_.has_value())
            udp_socket_.value().close();
    }

    void tcp_server_stream::do_write(IoBuffer event)
    {
        if (!use_udp_) {
            write_tcp(std::move(event));

            if (is_udp_enabled())
                use_udp_ = true;

            // A read request via TCP is needed in order to receive notification of the completion of
            // a connection with a client in SOCKS5 UDP_ASSOCIATE mode
            do_read();
        } else {
            write_udp(std::move(event));
        }
    }

    std::vector<std::uint8_t> tcp_server_stream::do_udp_associate()
    {
        udp::endpoint udp_bind_request_ep{socket_.local_endpoint().address(), 0};
        udp_socket_ = udp::socket(socket_.get_executor(), udp_bind_request_ep);
        return aux::endpoint_to_bytes(udp_socket_->local_endpoint());
    }

    void tcp_server_stream::do_read()
    {
        if (!is_udp_enabled()) {
            read_tcp();
        } else {
            read_udp();
        }
    }

    void tcp_server_stream::handle_error(const net::error_code& ec)
    {
        manager()->on_error(ec, shared_from_this());
    }

    void tcp_server_stream::write_udp(IoBuffer buffer)
    {
        // +---- + ------ + ------ + ---------- + -------- + ---------- +
        // | RSV |  FRAG  | ATYP   |  DST.ADDR  | DST.PORT |    DATA    |
        // +---- + ------ + ------ + ---------- + -------- + ---------- +
        // |  2  |    1   |   1    |  Variable  |    2     |  Variable  |
        // +---- + ------ + ------ + ---------- + -------- + ---------- +
        const auto client_addr_bytes = aux::endpoint_to_bytes(sender_ep_);
        udp_write_buffer_[0] = udp_read_buffer_[1] = udp_read_buffer_[2] = 0;
        udp_write_buffer_[3] = sender_ep_.address().is_v4() ? ATYPE_IPv4 : ATYPE_IPv6;

        const auto udp_reply_hdr_size = client_addr_bytes.size() + SOCKS5_UDP_HEADER_SIZE;

        std::ranges::copy(client_addr_bytes, udp_write_buffer_.data() + SOCKS5_UDP_HEADER_SIZE);
        std::ranges::copy(buffer, udp_write_buffer_.data() + udp_reply_hdr_size);

        (*udp_socket_).async_send_to(
            net::buffer(udp_write_buffer_.data(), buffer.size() + udp_reply_hdr_size), sender_ep_,
            [this, self{shared_from_this()}](const net::error_code& ec, std::size_t bytes_sent) {
            if (!ec)
                manager()->on_write(std::move(IoBuffer{}), shared_from_this());
            else
                handle_error(ec);
        });
    }

    void tcp_server_stream::write_tcp(IoBuffer buffer)
    {
        std::ranges::copy(buffer, write_buffer_.begin());
        net::async_write(
            socket_, net::buffer(write_buffer_, buffer.size()),
            [this, self{shared_from_this()}](const net::error_code& ec, size_t) {
                if (!ec)
                    manager()->on_write(std::move(IoBuffer{}), shared_from_this());
                else
                    handle_error(ec);
            });
    }

    void tcp_server_stream::read_udp()
    {
        (*udp_socket_).async_receive_from(
            net::buffer(udp_read_buffer_), sender_ep_,
            [this, self{shared_from_this()}](const net::error_code& ec, const size_t length)
            {
                if (!ec) {
                    uint8_t* data = udp_read_buffer_.data();
                    IoBuffer event(udp_read_buffer_.data(), udp_read_buffer_.data() + length);
                    manager()->on_read(std::move(event), shared_from_this());
                }
                else {
                    handle_error(ec);
                }
            });
    }

    void tcp_server_stream::read_tcp()
    {
        socket_.async_read_some(
            net::buffer(read_buffer_),
            [this, self{shared_from_this()}](const net::error_code& ec, const size_t length)
            {
                if (!ec) {
                    IoBuffer event(read_buffer_.data(), read_buffer_.data() + length);
                    manager()->on_read(std::move(event), shared_from_this());
                }
                else {
                    handle_error(ec);
                }
            });
    }
}