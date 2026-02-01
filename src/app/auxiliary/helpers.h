#ifndef MTLS_MPROXY_HELPERS_H
#define MTLS_MPROXY_HELPERS_H

#include <asio/ip/tcp.hpp>
#include <asio/ip/udp.hpp>

namespace aux {

    namespace net = asio;
    using tcp = asio::ip::tcp;
    using udp = asio::ip::udp;

    template <typename EpType>
    std::vector<std::uint8_t> endpoint_ip_to_bytes(const EpType& ep)
    {
        const auto& addr = ep.address();

        auto fn = [](auto&& bytes) {
            return std::vector<std::uint8_t>(bytes.begin(), bytes.end());
        };

        if (addr.is_v4())
            return fn(addr.to_v4().to_bytes());

        return fn(addr.to_v6().to_bytes());
    }

    template <typename EpType>
    std::vector<std::uint8_t> endpoint_to_bytes(const EpType& ep)
    {
        const auto& addr = ep.address();

        auto fn = [](auto&& bytes, std::uint16_t port) {
            std::vector<std::uint8_t> vec;
            vec.reserve(bytes.size() + 2);
            vec.assign(bytes.begin(), bytes.end());
            vec.push_back(port >> 8);
            vec.push_back(port & 0xff);
            return vec;
        };

        if (addr.is_v4())
            return fn(addr.to_v4().to_bytes(), ep.port());

        return fn(addr.to_v6().to_bytes(), ep.port());
    }

    template <typename EpType>
    std::string to_string(const EpType& ep)
    {
        return ep.address().to_string() + ":" + std::to_string(ep.port());
    }
}

#endif // MTLS_MPROXY_HELPERS_H