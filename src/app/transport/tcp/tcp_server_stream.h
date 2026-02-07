#ifndef MTLS_MPROXY_TRANSPORT_TCP_SERVER_STREAM_H
#define MTLS_MPROXY_TRANSPORT_TCP_SERVER_STREAM_H

#include "transport/server_stream.h"

#include <asynclog/logger_factory.h>

#include <asio/ip/tcp.hpp>
#include <asio/ip/udp.hpp>

#include <queue>

namespace mtls_mproxy
{
    namespace net = asio;
    using tcp = asio::ip::tcp;
    using udp = asio::ip::udp;

    class TcpServerStream final : public ServerStream
    {
    public:

        static std::shared_ptr<TcpServerStream> create(const StreamManagerPtr& ptr,
                                                       int id,
                                                       tcp::socket&& socket,
                                                       const asynclog::LoggerFactory& log_factory);
            ~TcpServerStream() override;

        net::any_io_executor executor() override;

    private:
        TcpServerStream(const StreamManagerPtr& ptr,
                        int id,
                        tcp::socket&& socket,
                        const asynclog::LoggerFactory& log_factory);

        void do_start() override;
        void do_stop() override;
        void do_read() override;
        void do_write(IoBuffer event) override;
        std::vector<std::uint8_t> do_udp_associate() override;

        void handle_error(const net::error_code& ec);
        bool is_udp_enabled() const { return udp_socket_.has_value(); }

        void write_udp();
        void write_tcp(IoBuffer buffer);

        void read_udp();
        void read_tcp();

        tcp::socket socket_;
        net::any_io_executor executor_;
        std::optional<udp::socket> udp_socket_ = std::nullopt;
        asynclog::ScopedLogger logger_;

        udp::endpoint sender_ep_;
        bool use_udp_{false};

        std::array<std::uint8_t, max_buffer_size> read_buffer_;
        std::array<std::uint8_t, max_buffer_size> write_buffer_;
        std::array<std::uint8_t, max_buffer_size> udp_read_buffer_;
        std::queue<IoBuffer> udp_write_queue_;
        bool udp_write_in_progress_{false};

        bool rip_{false};
        bool wip_{false};
    };
}

#endif // MTLS_MPROXY_TRANSPORT_TCP_SERVER_STREAM_H
