#ifndef MTLS_MPROXY_TRANSPORT_TLS_SERVER_STREAM_H
#define MTLS_MPROXY_TRANSPORT_TLS_SERVER_STREAM_H

#include "transport/server_stream.h"

#include <asio/ip/tcp.hpp>
#include <asio/ip/udp.hpp>
#include <asio/ssl.hpp>

#include <asynclog/logger_factory.h>
#include <asynclog/scoped_logger.h>

namespace mtls_mproxy
{
    namespace net = asio;
    using tcp = asio::ip::tcp;
    using udp = asio::ip::tcp;
    using ssl_socket = net::ssl::stream<net::ip::tcp::socket>;

    class TlsServerStream final
        : public ServerStream
        , public std::enable_shared_from_this<TlsServerStream>
    {
    public:
        TlsServerStream(const StreamManagerPtr& ptr,
                        int id,
                        ssl_socket&& socket,
                        const asynclog::LoggerFactory& log_factory);
        ~TlsServerStream() override;

        net::any_io_executor executor() override;

        void start() override;
        void stop() override;
        void read() override;
        void write(IoBuffer event) override;
        std::vector<std::uint8_t> udp_associate() override;

    private:
        void do_handshake();

        void handle_error(const net::error_code& ec);

        ssl_socket socket_;
        std::optional<udp::socket> udp_socket_;
        asynclog::ScopedLogger logger_;

        std::array<std::uint8_t, max_buffer_size> read_buffer_;
        std::array<std::uint8_t, max_buffer_size> write_buffer_;
    };
}
#endif // MTLS_MPROXY_TRANSPORT_TLS_SERVER_STREAM_H
