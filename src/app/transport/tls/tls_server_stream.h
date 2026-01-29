#ifndef MTLS_MPROXY_TRANSPORT_TLS_SERVER_STREAM_H
#define MTLS_MPROXY_TRANSPORT_TLS_SERVER_STREAM_H

#include "transport/server_stream.h"

#include <asio.hpp>
#include <asio/ssl.hpp>

#include <asynclog/logger_factory.h>
#include <asynclog/scoped_logger.h>

namespace mtls_mproxy
{
    namespace net = asio;
    using tcp = asio::ip::tcp;
    using ssl_socket = net::ssl::stream<net::ip::tcp::socket>;

    class TlsServerStream final : public ServerStream
    {
    public:
        TlsServerStream(const StreamManagerPtr& ptr,
                        int id,
                        ssl_socket&& socket,
                        const asynclog::LoggerFactory& log_factory);
        ~TlsServerStream() override;

        net::any_io_executor executor() override;
        tcp::socket& socket();
    private:
        void do_handshake();
        void do_start() override;
        void do_stop() override;
        void do_read() override;
        void do_write(IoBuffer event) override;

        void handle_error(const net::error_code& ec);

        ssl_socket socket_;
        asynclog::ScopedLogger logger_;

        std::array<std::uint8_t, max_buffer_size> read_buffer_;
        std::array<std::uint8_t, max_buffer_size> write_buffer_;
    };
}
#endif // MTLS_MPROXY_TRANSPORT_TLS_SERVER_STREAM_H
