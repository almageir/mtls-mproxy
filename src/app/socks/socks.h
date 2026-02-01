#ifndef MTLS_MPROXY_SOCKS_H
#define MTLS_MPROXY_SOCKS_H

#include <iostream>
#include <cstdint>
#include <optional>

namespace proto {
    constexpr std::int8_t IPV4_ADDR_SIZE = 4;
    constexpr std::int8_t IPV6_ADDR_SIZE = 16;
    constexpr std::int8_t TCP_PORT_SIZE = 2;

    enum
    {
        version = 0x05,
        ipv4 = 0x01,
        dom = 0x03,
        ipv6 = 0x04,
        reserved = 0x00,

        max_dom_length = 255,
        dom_length_field_size = 1,
        dom_length_field_offset = 0,
        dom_field_offset = 1,

        request_header_min_length = 10
    };

    enum AuthMethod : std::uint8_t {
        NoAuth = 0x00,
        GssApi = 0x01,
        UserPass = 0x02,
        NotSupported = 0xff,
        MaxNumberOfMethods = 0xff,
        HeaderMinlength = 0x02,
    };
}

class Socks
{
public:
    enum Request : std::uint8_t {
        tcp_connection = 0x01,
        tcp_port = 0x02,
        udp_port = 0x03
    };

    enum Responses : std::uint8_t {
        succeeded = 0x00,
        general_socks_server_failure = 0x01,
        connection_not_allowed_by_ruleset = 0x02,
        network_unreachable = 0x03,
        host_unreachable = 0x04,
        connection_refused = 0x05,
        ttl_expired = 0x06,
        command_not_supported = 0x07,
        address_type_not_supported = 0x08
    };

    static constexpr int kMaxHostInfoSize = proto::max_dom_length + proto::dom_length_field_size + proto::TCP_PORT_SIZE;

    struct RequestHeader {
        std::uint8_t version;
        std::uint8_t command;
        std::uint8_t reserved;
        std::uint8_t type;
        std::uint8_t data[kMaxHostInfoSize];
    };

    struct AuthRequestHeader {
        std::uint8_t version;
        std::uint8_t number_of_methods;
        std::uint8_t methods[proto::AuthMethod::MaxNumberOfMethods];
    };

    static std::optional<std::string> is_socks5_auth_request(const std::uint8_t* buffer, std::size_t length);
    static std::optional<Request> parse_requested_socks_mode(const std::uint8_t* buffer, std::size_t length);
    static bool get_remote_address_info(const std::uint8_t* buffer, std::size_t length, std::string& host, std::string& port);
    static uint16_t get_port_from_binary(const std::uint8_t* buffer);
};


#endif // MTLS_MPROXY_SOCKS_H
