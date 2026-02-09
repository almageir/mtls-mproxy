#include "tcp_server_stream.h"

#include <iostream>

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
        const auto rep = sock.remote_endpoint(ec);

        if (ec)
            return std::string{"remote_endpoint failed: " + ec.message()};

        return aux::to_string(rep);
    }
}

namespace mtls_mproxy
{
    TcpServerStream::TcpServerStream(const StreamManagerPtr& ptr,
                                     int id,
                                     tcp::socket&& socket,
                                     const asynclog::LoggerFactory& log_factory)
        : ServerStream{ptr, id}
        , socket_{std::move(socket)}
        , executor_{socket_.get_executor()}
        , logger_{log_factory.create("tcp_server_stream")}
        , read_buffer_{}
        , write_buffer_{}
        , udp_read_buffer_{}
    {
    }

    std::shared_ptr<TcpServerStream> TcpServerStream::create(const StreamManagerPtr& ptr,
                                                             int id,
                                                             tcp::socket&& socket,
                                                             const asynclog::LoggerFactory& log_factory)
    {
        return std::shared_ptr<TcpServerStream>(
            new TcpServerStream(ptr, id, std::move(socket), log_factory));
    }

    TcpServerStream::~TcpServerStream()
    {
        logger_.debug(std::format("[{}] tcp server stream closed", id()));
    }

    net::any_io_executor TcpServerStream::executor() { return socket_.get_executor(); }

    void TcpServerStream::start()
    {
        logger_.debug(std::format("[{}] incoming connection from client: [{}]", id(), ep_to_str(socket_)));
        net::post(executor_, [self{shared_from_this()}]() {
            self->manager()->on_server_ready(self);
        });
    }

    void TcpServerStream::stop()
    {
        net::error_code ignored_ec;
        socket_.shutdown(tcp::socket::shutdown_both, ignored_ec);
        if (udp_socket_.has_value())
            udp_socket_.value().close();
    }

    void TcpServerStream::write(IoBuffer event)
    {
        if (!use_udp_) {
            write_tcp(std::move(event));

            if (is_udp_enabled()) {
                use_udp_ = true;
                // A read request via TCP is needed in order to receive notification of the completion of
                // a connection with a client in SOCKS5 UDP_ASSOCIATE mode
                read_tcp();
                // And here the UDP reading chain will be launched
                read();
            }
        } else {
            IoBuffer packet{};

            const auto client_addr_bytes = aux::endpoint_to_bytes(sender_ep_);
            const auto udp_reply_hdr_size = client_addr_bytes.size() + SOCKS5_UDP_HEADER_SIZE;

            packet.resize(udp_reply_hdr_size + event.size());
            packet[0] = packet[1] = packet[2] = 0;
            packet[3] = sender_ep_.address().is_v4() ? ATYPE_IPv4 : ATYPE_IPv6;

            std::ranges::copy(client_addr_bytes, packet.data() + SOCKS5_UDP_HEADER_SIZE);
            std::ranges::copy(event, packet.data() + udp_reply_hdr_size);

            udp_write_queue_.emplace(std::move(packet));

            write_udp();
        }
    }

    std::vector<std::uint8_t> TcpServerStream::udp_associate()
    {
        udp::endpoint udp_bind_request_ep{socket_.local_endpoint().address(), 0};
        udp_socket_ = udp::socket(socket_.get_executor(), udp_bind_request_ep);
        return aux::endpoint_to_bytes(udp_socket_->local_endpoint());
    }

    void TcpServerStream::read()
    {
        if (!is_udp_enabled()) {
            read_tcp();
        } else {
            read_udp();
        }
    }

    void TcpServerStream::handle_error(const net::error_code& ec)
    {
        manager()->on_error(ec, shared_from_this());
    }

    // +---- + ------ + ------ + ---------- + -------- + ---------- +
    // | RSV |  FRAG  | ATYP   |  DST.ADDR  | DST.PORT |    DATA    |
    // +---- + ------ + ------ + ---------- + -------- + ---------- +
    // |  2  |    1   |   1    |  Variable  |    2     |  Variable  |
    // +---- + ------ + ------ + ---------- + -------- + ---------- +
    void TcpServerStream::write_udp()
    {
        if (udp_write_queue_.empty())
            return;

        if (udp_write_in_progress_)
            return;

        const auto& packet = udp_write_queue_.front();
        udp_write_in_progress_ = true;

        (*udp_socket_).async_send_to(
            net::buffer(packet.data(), packet.size()), sender_ep_,
                [this, self{shared_from_this()}](const net::error_code& ec, std::size_t bytes_sent) {
                    if (!ec) {
                        udp_write_queue_.pop();
                        udp_write_in_progress_ = false;
                        manager()->on_write(self);
                        if (!udp_write_queue_.empty())
                            write_udp();
                    }
                    else {
                        handle_error(ec);
                    }
                });
    }

    void TcpServerStream::write_tcp(IoBuffer buffer)
    {
        if (wip_) {
            logger_.debug(std::format("[{}] write in progress", id()));
            return;
        }
        wip_ = true;
        std::ranges::copy(buffer, write_buffer_.begin());
        net::async_write(
            socket_, net::buffer(write_buffer_, buffer.size()),
            [this, self{shared_from_this()}](const net::error_code& ec, size_t) {
                if (!ec) {
                    wip_ = false;
                    manager()->on_write(self);
                } else
                    handle_error(ec);
            });
    }

    void TcpServerStream::read_udp()
    {
        (*udp_socket_).async_receive_from(
            net::buffer(udp_read_buffer_), sender_ep_,
            [this, self{shared_from_this()}](const net::error_code& ec, const size_t length)
            {
                if (!ec) {
                    uint8_t* data = udp_read_buffer_.data();
                    IoBuffer event(udp_read_buffer_.data(), udp_read_buffer_.data() + length);
                    manager()->on_read(std::move(event), self);
                }
                else {
                    handle_error(ec);
                }
            });
    }

    void TcpServerStream::read_tcp()
    {
        if (rip_) {
            logger_.debug(std::format("[{}] read in progress", id()));
            return;
        }
        rip_ = true;
        socket_.async_read_some(
            net::buffer(read_buffer_),
            [this, self{shared_from_this()}](const net::error_code& ec, const size_t length)
            {
                if (!ec) {
                    rip_ = false;
                    IoBuffer event(read_buffer_.data(), read_buffer_.data() + length);
                    manager()->on_read(std::move(event), self);
                }
                else {
                    handle_error(ec);
                }
            });
    }
}