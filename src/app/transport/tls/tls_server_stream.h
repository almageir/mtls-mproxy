#ifndef TLS_SERVER_STREAM_H
#define TLS_SERVER_STREAM_H

#include "transport/server_stream.h"

#include <asio.hpp>
#include <asio/ssl.hpp>

#include <asynclog/logger_factory.h>
#include <asynclog/scoped_logger.h>

namespace net = asio;
using tcp = asio::ip::tcp;
using ssl_socket = net::ssl::stream<net::ip::tcp::socket>;

class tls_server_stream final : public server_stream
{
public:
    tls_server_stream(const stream_manager_ptr& ptr, int id, net::io_context& ctx, net::ssl::context& ssl_ctx, asynclog::LoggerFactory log_factory);
    ~tls_server_stream() override;

    net::io_context& context() override;
    tcp::socket& socket();
private:
    void do_handshake();
    void do_start() override;
    void do_stop() override;
    void do_read() override;
    void do_write(io_buffer event) override;

    void handle_error(const net::error_code& ec);

    net::io_context& ctx_;
    net::ssl::context& ssl_ctx_;
    ssl_socket socket_;
    asynclog::ScopedLogger logger_;

    std::array<std::uint8_t, max_buffer_size> read_buffer_;
    std::array<std::uint8_t, max_buffer_size> write_buffer_;
};

#endif //TLS_SERVER_STREAM_H
