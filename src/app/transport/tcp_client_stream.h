#ifndef MTLS_MPROXY_TRANSPORT_TCP_CLIENT_STREAM_H
#define MTLS_MPROXY_TRANSPORT_TCP_CLIENT_STREAM_H

#include "client_stream.h"

#include <asynclog/logger_factory.h>

#include <asio/ip/tcp.hpp>

namespace mtls_mproxy
{
    namespace net = asio;
    using tcp = asio::ip::tcp;

    class TcpClientStream final
        : public ClientStream
        , public std::enable_shared_from_this<TcpClientStream>
    {
    public:
        ~TcpClientStream() override;

        static std::shared_ptr<TcpClientStream> create(
            const StreamManagerPtr& ptr,
            int id,
            net::any_io_executor ctx,
            const asynclog::LoggerFactory& log_factory);

        void start() override;
        void stop() override;
        void read() override;
        void write(IoBuffer event) override;

        void connect(tcp::resolver::results_type&& results);

    private:
        TcpClientStream(const StreamManagerPtr& ptr,
                        int id,
                        net::any_io_executor ctx,
                        const asynclog::LoggerFactory& log_factory);


        void handle_error(const net::error_code& ec);

        void set_host(std::string host) override;
        void set_service(std::string service) override;

        tcp::socket socket_;
        tcp::resolver resolver_;

        asynclog::ScopedLogger logger_;

        std::string host_;
        std::string port_;

        std::array<std::uint8_t, max_buffer_size> read_buffer_;
        std::array<std::uint8_t, max_buffer_size> write_buffer_;

        bool rip_{false};
        bool wip_{false};
    };
}

#endif // MTLS_MPROXY_TRANSPORT_TCP_CLIENT_STREAM_H
