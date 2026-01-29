#ifndef MTLS_MPROXY_TRANSPORT_TCP_CLIENT_STREAM_H
#define MTLS_MPROXY_TRANSPORT_TCP_CLIENT_STREAM_H

#include "client_stream.h"

#include <asynclog/logger_factory.h>

#include <asio/ip/tcp.hpp>

namespace mtls_mproxy
{
    namespace net = asio;
    using tcp = asio::ip::tcp;

    class TcpClientStream final : public ClientStream
    {
    public:
        TcpClientStream(const StreamManagerPtr& ptr,
                        int id,
                        net::any_io_executor ctx,
                        const asynclog::LoggerFactory& log_factory);
        ~TcpClientStream() override;

    private:
        void do_start() override;
        void do_stop() override;

        void do_connect(tcp::resolver::results_type&& results);

        void do_read() override;
        void do_write(IoBuffer event) override;

        void handle_error(const net::error_code& ec);

        void do_set_host(std::string host) override;
        void do_set_service(std::string service) override;

        tcp::socket socket_;
        tcp::resolver resolver_;

        asynclog::ScopedLogger logger_;

        std::string host_;
        std::string port_;

        std::array<std::uint8_t, max_buffer_size> read_buffer_;
        std::array<std::uint8_t, max_buffer_size> write_buffer_;
    };
}

#endif // MTLS_MPROXY_TRANSPORT_TCP_CLIENT_STREAM_H
