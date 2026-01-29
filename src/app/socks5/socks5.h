#ifndef SOCKS5_HPP
#define SOCKS5_HPP

#include <iostream>
#include <cstdint>
#include <optional>

namespace proto {
    namespace auth {
        enum : std::uint8_t {
            kNoAuth = 0x00,
            kGssApi = 0x01,
            kUserPass = 0x02,
            kNotSupported = 0xff,
            kMaxNumberOfMethods = 0xff,
            kHeaderMinlength = 0x02,
        };

        struct request {
            std::uint8_t version;
            std::uint8_t number_of_methods;
            std::uint8_t methods[auth::kMaxNumberOfMethods];
        };
    }
}

class socks5
{
public:
    enum proto {
        version = 0x05,
        ipv4 = 0x01,
        dom = 0x03,
        ipv6 = 0x04,
        reserved = 0x00,

        max_dom_length = 255,
        dom_length_field_size = 1,
        dom_length_field_offset = 0,
        dom_field_offset = 1,

        port_length = 2,
        ipv4_length = 4,
        ipv6_length = 16,

        request_header_min_length = 10,
        max_host_info_size = max_dom_length + dom_length_field_size + port_length
    };

    enum request : std::uint8_t {
        tcp_connection = 0x01,
        tcp_port = 0x02,
        udp_port = 0x03
    };

    enum responses : std::uint8_t {
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

    struct request_header {
        std::uint8_t version;
        std::uint8_t command;
        std::uint8_t reserved;
        std::uint8_t type;
        std::uint8_t data[max_host_info_size];
    };

    static std::optional<std::string> is_socks5_auth_request(const std::uint8_t* buffer, std::size_t length);

    static bool is_valid_request_packet(const std::uint8_t* buffer, std::size_t length);

    static bool get_remote_address_info(const std::uint8_t* buffer, std::size_t length, std::string& host, std::string& port);

    static uint16_t get_port_from_binary(const std::uint8_t* buffer);
};


#endif // SOCKS5_HPP
