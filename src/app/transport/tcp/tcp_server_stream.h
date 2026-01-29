#ifndef MTLS_MPROXY_TRANSPORT_TCP_SERVER_STREAM_H
#define MTLS_MPROXY_TRANSPORT_TCP_SERVER_STREAM_H

#include "transport/server_stream.h"

#include <asio.hpp>

#include "asynclog/logger_factory.h"

namespace mtls_mproxy
{
    namespace net = asio;
    using tcp = asio::ip::tcp;

    class tcp_server_stream final : public ServerStream
    {
    public:
        tcp_server_stream(const StreamManagerPtr& ptr,
                          int id,
                          tcp::socket&& socket,
                          const asynclog::LoggerFactory& log_factory);
        ~tcp_server_stream() override;

        net::any_io_executor executor() override;
        tcp::socket& socket();
    private:
        void do_start() override;
        void do_stop() override;
        void do_read() override;
        void do_write(IoBuffer event) override;

        void handle_error(const net::error_code& ec);

        tcp::socket socket_;
        asynclog::ScopedLogger logger_;

        std::array<std::uint8_t, max_buffer_size> read_buffer_;
        std::array<std::uint8_t, max_buffer_size> write_buffer_;
    };
}

#endif // MTLS_MPROXY_TRANSPORT_TCP_SERVER_STREAM_H
