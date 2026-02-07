#include "udp_client_stream.h"
#include "stream_manager.h"
#include "auxiliary/helpers.h"

#include <asio/write.hpp>
#include <asio/connect.hpp>
#include <socks/socks.h>

namespace
{
    namespace net = asio;
    using udp = asio::ip::udp;

    constexpr std::uint8_t ATYPE_IPv4 = 0x01;
    constexpr std::uint8_t ATYPE_DOMAIN = 0x03;
    constexpr std::uint8_t ATYPE_IPv6 = 0x04;
    constexpr std::size_t SOCKS5_UDP_HEADER_SIZE = 0x04;

    // +---- + ------ + ------ + ---------- + -------- + ---------- +
    // | RSV |  FRAG  | ATYP   |  DST.ADDR  | DST.PORT |    DATA    |
    // +---- + ------ + ------ + ---------- + -------- + ---------- +
    // |  2  |    1   |   1    |  Variable  |    2     |  Variable  |
    // +---- + ------ + ------ + ---------- + -------- + ---------- +
    std::size_t determine_udp_data_offset(mtls_mproxy::IoBuffer buffer)
    {
        if (buffer.size() < 10)
            return 0;

        if (buffer[3] == ATYPE_IPv4)
            return 10ull;

        if (buffer[3] == ATYPE_IPv6)
            return 22ull;

        std::size_t offset = 4;
        offset += buffer[4]; // domain length
        offset++;            // domain length field length
        offset += 2;         // port length

        if (offset < buffer.size())
            return offset;

        return 0;
    }

    enum : std::int32_t { eRemote, eLocal };
    std::string ep_to_str(const udp::socket& sock, std::int32_t dir)
    {
        if (!sock.is_open())
            return "socket not opened";

        net::error_code ec;
        const auto& rep = (dir == eRemote) ? sock.remote_endpoint(ec) : sock.local_endpoint(ec);
        if (ec)
        {
            return std::format("{}: {}",
                               ((dir == eRemote) ? "remote_endpoint failed" : "local_endpoint failed"),
                               ec.message());
        }

        return  aux::to_string(rep);
    }
}

namespace mtls_mproxy
{
    UdpClientStream::UdpClientStream(const StreamManagerPtr& ptr,
                                     int id, net::any_io_executor ctx,
                                     const asynclog::LoggerFactory& logger_factory)
        : ClientStream{ptr, id}
        , socket_{ctx, udp::v4()}
        , resolver_{ctx}
        , logger_{logger_factory.create("udp_client")}
        , read_buffer_{}/*, write_buffer_{}*/
    {
    }

    UdpClientStream::~UdpClientStream()
    {
        logger_.debug(std::format("[{}] udp client stream closed", id()));
    }

    void UdpClientStream::do_start() {
        socket_.bind(udp::endpoint{udp::v4(), 0});
        do_read();
    }

    void UdpClientStream::do_stop()
    {
        net::error_code ignored_ec;
        socket_.shutdown(udp::socket::shutdown_both, ignored_ec);
    }

    void UdpClientStream::do_write(IoBuffer event)
    {
        std::size_t data_offset = determine_udp_data_offset(event);

        std::string addr, port;
        if (!Socks::get_remote_address_info(event.data(), event.size(), addr, port) || data_offset == 0) {
            logger_.warn(std::format("[{}] invalide address requested", id()));
            return;
        }

        std::size_t bytes_to_send = event.size() - data_offset;

        Packet packet;
        packet.data.resize(bytes_to_send);
        std::copy(event.begin() + data_offset, event.end(), packet.data.begin());
        packet.addr = addr;
        packet.port = port;

        if ((event[3] == ATYPE_IPv4 || event[3] == ATYPE_IPv6)) {
            write_queue_.emplace(std::move(packet));
            write_packet();
        } else {
            dns_queue_.emplace(std::move(packet));
            make_dns_resolve();
        }
    }

    void UdpClientStream::write_packet()
    {
        if (write_queue_.empty())
            return;

        if (write_in_progress_)
            return;

        const auto& packet = write_queue_.front();
        write_in_progress_ = true;

        logger_.info(std::format("[{}] requested ip address [{}:{}]", id(), packet.addr, packet.port));

        udp::endpoint target_endpoint(asio::ip::make_address(packet.addr), std::stoi(packet.port));

        socket_.async_send_to(
            net::buffer(packet.data, packet.data.size()),
            target_endpoint,
            [this, self{shared_from_this()}](const net::error_code& ec, std::size_t bytes_sent) {
                if (!ec) {
                    write_queue_.pop();
                    write_in_progress_ = false;
                    manager()->on_write(shared_from_this());
                    if (!write_queue_.empty())
                        write_packet();
                } else {
                    write_in_progress_ = false;
                    handle_error(ec);
                }
            });
    }

    void UdpClientStream::make_dns_resolve()
    {
        if (dns_queue_.empty())
            return;

        if (resolve_in_progress_)
            return;

        const auto& packet = dns_queue_.front();
        resolve_in_progress_ = true;

        logger_.info(std::format("[{}] requested domain address [{}:{}]", id(), packet.addr, packet.port));
        resolver_.async_resolve(
            packet.addr, packet.port,
            [this, self{shared_from_this()}](const net::error_code& ec, udp::resolver::results_type results) {
                if (!ec) {
                    auto packet = dns_queue_.front();
                    udp::endpoint ep{results.begin()->endpoint().address(),
                        static_cast<std::uint16_t>(std::stoi(packet.port))};

                    logger_.info(std::format("[{}] resolved domain address [{}:{}] -> [{}]", id(),
                                             packet.addr, packet.port, aux::to_string(ep)));

                    packet.addr = ep.address().to_string();

                    write_queue_.emplace(std::move(packet));
                    dns_queue_.pop();
                    resolve_in_progress_ = false;
                    if (!dns_queue_.empty())
                        make_dns_resolve();
                } else {
                    resolve_in_progress_ = false;
                    handle_error(ec);
                }
            });
    }

    void UdpClientStream::do_read()
    {
        socket_.async_receive_from(
            net::buffer(read_buffer_), sender_ep_,
            [this, self{shared_from_this()}](const net::error_code& ec, const size_t length) {
                if (!ec) {
                    uint8_t* data = read_buffer_.data();
                    IoBuffer event(read_buffer_.data(), read_buffer_.data() + length);
                    manager()->on_read(std::move(event), shared_from_this());
                }
                else {
                    handle_error(ec);
                }
            });
    }

    void UdpClientStream::handle_error(const net::error_code& ec)
    {
        manager()->on_error(ec, shared_from_this());
    }

    void UdpClientStream::do_set_host(std::string host) {}

    void UdpClientStream::do_set_service(std::string service) {}
}