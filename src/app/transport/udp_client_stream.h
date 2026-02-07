#ifndef MTLS_MPROXY_TRANSPORT_UDP_CLIENT_STREAM_H
#define MTLS_MPROXY_TRANSPORT_UDP_CLIENT_STREAM_H

#include "client_stream.h"

#include <asynclog/logger_factory.h>

#include <asio/ip/udp.hpp>
#include <queue>

namespace mtls_mproxy
{
    namespace net = asio;
    using udp = asio::ip::udp;

    struct Packet {
        IoBuffer data;
        std::string addr;
        std::string port;
    };

    class UdpClientStream final
        : public ClientStream
        , public std::enable_shared_from_this<UdpClientStream>
    {
    public:
        UdpClientStream(const StreamManagerPtr& ptr,
                        int id,
                        const net::any_io_executor &ctx,
                        const asynclog::LoggerFactory& log_factory);
        ~UdpClientStream() override;

        void start() override;
        void stop() override;
        void read() override;
        void write(IoBuffer event) override;

        void set_host(std::string host) override;
        void set_service(std::string service) override;

    private:
        void write_packet();
        void make_dns_resolve();
        void handle_error(const net::error_code& ec);

        udp::socket socket_;
        udp::resolver resolver_;

        asynclog::ScopedLogger logger_;

        udp::endpoint sender_ep_;

        std::array<std::uint8_t, max_buffer_size> read_buffer_;

        std::queue<Packet> write_queue_;
        std::queue<Packet> dns_queue_;

        bool write_in_progress_{false};
        bool resolve_in_progress_{false};
    };
}

#endif // MTLS_MPROXY_TRANSPORT_UDP_CLIENT_STREAM_H
