#ifndef TCP_SERVER_STREAM_H
#define TCP_SERVER_STREAM_H

#include "transport/server_stream.h"

#include <asio.hpp>

#include "asynclog/logger_factory.h"

namespace net = asio;
using tcp = asio::ip::tcp;

class tcp_server_stream final : public server_stream
{
public:
    tcp_server_stream(const stream_manager_ptr& ptr, int id, net::io_context& ctx, asynclog::LoggerFactory log_factory);
    ~tcp_server_stream() override;

    net::io_context& context() override;
    tcp::socket& socket();
private:
    void do_start() final;
    void do_stop() final;
    void do_read() final;
    void do_write(io_buffer event) final;

    void handle_error(const net::error_code& ec);

    net::io_context& ctx_;
    tcp::socket socket_;

    asynclog::ScopedLogger logger_;

    std::array<std::uint8_t, max_buffer_size> read_buffer_;
    std::array<std::uint8_t, max_buffer_size> write_buffer_;
};

#endif //TCP_SERVER_STREAM_H
