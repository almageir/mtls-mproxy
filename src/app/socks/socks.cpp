#include "socks.h"

#include <asio/ip/address.hpp>

namespace net = asio;

std::optional<std::string> Socks::is_socks5_auth_request(const std::uint8_t* buffer, std::size_t length)
{
    if (!buffer || length < proto::AuthMethod::HeaderMinlength)
        return "socks5 proto: bad initial request packet";

    const auto auth_header = static_cast<const AuthRequestHeader*>(static_cast<const void*>(buffer));
    if (auth_header->version != proto::version)
        return "socks5 proto: only protocol version 5 is supported";

    if (static_cast<size_t>(auth_header->number_of_methods + proto::AuthMethod::HeaderMinlength) > length)
        return "socks5 proto: invalid authentication request length";

    for (std::size_t i=0; i<auth_header->number_of_methods; i++)
        if (auth_header->methods[i] == proto::AuthMethod::NoAuth)
            return {};

    return "authentication support is not implemented";
}

std::optional<Socks::Request> Socks::parse_requested_socks_mode(const std::uint8_t *buffer, std::size_t length)
{
    if (buffer && length >= proto::request_header_min_length) {
        const auto* request = reinterpret_cast<const RequestHeader*>(buffer);

        if (request->version != proto::version)
            return std::nullopt;

        if (request->command != Request::tcp_connection && request->command != Request::udp_port)
            return std::nullopt;

        if (!(request->type == proto::ipv4 || request->type == proto::ipv6 || request->type == proto::dom))
            return std::nullopt;

        return static_cast<Request>(request->command);
    }

    return std::nullopt;
}

bool Socks::get_remote_address_info(const std::uint8_t *buffer, std::size_t length, std::string &host, std::string &port)
{
    if (!buffer)
        return false;

    if (length >= proto::request_header_min_length) {
        const auto req = reinterpret_cast<const RequestHeader*>(buffer);

        net::error_code ec;
        char host_buffer[proto::max_dom_length] = {0};
        auto binaryAddress = reinterpret_cast<const char*>(&req->data[0]);

        if (req->type == proto::ipv4) {
            net::detail::socket_ops::inet_ntop(AF_INET, binaryAddress,
                                               host_buffer, proto::max_dom_length, 0, ec);
            if (!ec) {
                host.assign(host_buffer);
                port = std::move(std::to_string(get_port_from_binary(req->data + proto::IPV4_ADDR_SIZE)));
            }
        } else if (req->type == proto::ipv6) {
            net::detail::socket_ops::inet_ntop(AF_INET6, binaryAddress,
                                               host_buffer, proto::max_dom_length, 0, ec);
            if (!ec) {
                host.assign(host_buffer);
                port = std::move(std::to_string(get_port_from_binary(req->data + proto::IPV6_ADDR_SIZE)));
            }
        } else if (req->type == proto::dom) {
            auto domainLength = static_cast<std::size_t>(req->data[proto::dom_length_field_offset]);
            if (domainLength <= proto::max_dom_length) {
                host.assign(reinterpret_cast<const char*>(&req->data[proto::dom_field_offset]), domainLength);
                port = std::move(std::to_string(get_port_from_binary(req->data + domainLength + 1)));
            }
        } else {
            return false;
        }

        if (!ec && !host.empty() && !port.empty())
            return true;
    }

    return false;
}

std::uint16_t Socks::get_port_from_binary(const std::uint8_t* buffer)
{
    if (buffer)
        return static_cast<std::uint16_t>((buffer[0] << 8) | buffer[1]);

    return 0;
}
