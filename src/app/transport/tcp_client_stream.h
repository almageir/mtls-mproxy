#ifndef TCP_CLIENT_STREAM_H
#define TCP_CLIENT_STREAM_H

#include "client_stream.h"

#include <asio.hpp>

#include <asynclog/logger_factory.h>

namespace net = asio;
using tcp = asio::ip::tcp;

class tcp_client_stream final : public client_stream
{
public:
    tcp_client_stream(const stream_manager_ptr& ptr, int id, net::io_context& ctx, asynclog::LoggerFactory log_factory);
    ~tcp_client_stream() override;

private:
    void do_start() override;
    void do_stop() override;

    void do_connect(tcp::resolver::results_type&& results);

    void do_read() override;
    void do_write(io_buffer event) override;

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

#endif //TCP_CLIENT_STREAM_H
