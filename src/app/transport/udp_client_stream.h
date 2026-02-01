#ifndef MTLS_MPROXY_TRANSPORT_UDP_CLIENT_STREAM_H
#define MTLS_MPROXY_TRANSPORT_UDP_CLIENT_STREAM_H

#include "client_stream.h"

#include <asynclog/logger_factory.h>

#include <asio/ip/udp.hpp>

namespace mtls_mproxy
{
    namespace net = asio;
    using udp = asio::ip::udp;

    class UdpClientStream final : public ClientStream
    {
    public:
        UdpClientStream(const StreamManagerPtr& ptr,
                        int id,
                        net::any_io_executor ctx,
                        const asynclog::LoggerFactory& log_factory);
        ~UdpClientStream() override;

    private:
        void do_start() override;
        void do_stop() override;

        void do_read() override;
        void do_write(IoBuffer event) override;

        void handle_error(const net::error_code& ec);

        void do_set_host(std::string host) override;
        void do_set_service(std::string service) override;

        udp::socket socket_;
        udp::resolver resolver_;

        asynclog::ScopedLogger logger_;

        std::string host_;
        std::string port_;

        udp::endpoint sender_ep_;

        std::array<std::uint8_t, max_buffer_size> read_buffer_;
        std::array<std::uint8_t, max_buffer_size> write_buffer_;
    };
}

#endif // MTLS_MPROXY_TRANSPORT_UDP_CLIENT_STREAM_H
