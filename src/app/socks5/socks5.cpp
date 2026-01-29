#include "socks5.h"

#include <asio.hpp>

namespace net = asio;

using namespace proto;

std::optional<std::string> socks5::is_socks5_auth_request(const std::uint8_t *buffer, std::size_t length)
{
    if (!buffer || length < auth::kHeaderMinlength)
        return "socks5 proto: bad initial request packet";

    const auto auth_header = static_cast<const auth::request*>(static_cast<const void*>(buffer));
    if (auth_header->version != proto::version)
        return "socks5 proto: only protocol version 5 is supported";

    if (static_cast<size_t>(auth_header->number_of_methods + auth::kHeaderMinlength) > length)
        return "socks5 proto: invalid authentication request length";

    for (std::size_t i=0; i<auth_header->number_of_methods; i++)
        if (auth_header->methods[i] == auth::kNoAuth)
            return {};

    return "authentication support is not implemented";
}

bool socks5::is_valid_request_packet(const std::uint8_t *buffer, std::size_t length)
{
    if (buffer && length >= proto::request_header_min_length) {
        const auto* request = reinterpret_cast<const request_header*>(buffer);

        if (request->version != proto::version)
            return false;

        if (request->command != request::tcp_connection)
            return false;

        if (!(request->type == proto::ipv4 || request->type == proto::ipv6 || request->type == proto::dom))
            return false;

        return true;
    }

    return false;
}

bool socks5::get_remote_address_info(const std::uint8_t *buffer, std::size_t length, std::string &host, std::string &port)
{
    if (!buffer)
        return false;

    if (length >= proto::request_header_min_length) {
        auto req = reinterpret_cast<const request_header*>(buffer);

        net::error_code ec;
        char host_buffer[proto::max_dom_length] = {0};
        auto binaryAddress = reinterpret_cast<const char*>(&req->data[0]);

        if (req->type == ipv4) {
            net::detail::socket_ops::inet_ntop(AF_INET, binaryAddress,
                                               host_buffer, max_dom_length, 0, ec);
            if (!ec) {
                host.assign(host_buffer);
                port = move(std::to_string(get_port_from_binary(req->data + ipv4_length)));
            }
        } else if (req->type == ipv6) {
            net::detail::socket_ops::inet_ntop(AF_INET6, binaryAddress,
                                               host_buffer, max_dom_length, 0, ec);
            if (!ec) {
                host.assign(host_buffer);
                port = move(std::to_string(get_port_from_binary(req->data + ipv6_length)));
            }
        } else if (req->type == dom) {
            auto domainLength = static_cast<std::size_t>(req->data[dom_length_field_offset]);
            if (domainLength <= proto::max_dom_length) {
                host.assign(reinterpret_cast<const char *>(&req->data[dom_field_offset]), domainLength);
                port = move(std::to_string(get_port_from_binary(req->data + domainLength + 1)));
            }
        } else {
            return false;
        }

        if (!ec && !host.empty() && !port.empty())
            return true;
    }

    return false;
}

std::uint16_t socks5::get_port_from_binary(const std::uint8_t* buffer)
{
    if (buffer)
        return static_cast<std::uint16_t>((buffer[0] << 8) | buffer[1]);

    return 0;
}
