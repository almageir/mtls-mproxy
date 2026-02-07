#ifndef MTLS_MPROXY_TRANSPORT_SERVER_STREAM_H
#define MTLS_MPROXY_TRANSPORT_SERVER_STREAM_H

#include "transport/io_buffer.h"

#include <asio/any_io_executor.hpp>

#include <memory>

namespace mtls_mproxy
{
    namespace net = asio;

    class StreamManager;
    using StreamManagerPtr = std::shared_ptr<StreamManager>;

    class ServerStream
    {
    public:
        explicit ServerStream(StreamManagerPtr smp, int id = 0)
            : stream_manager_(std::move(smp))
            , id_(id)
        {}
        virtual ~ServerStream() = default;

        virtual net::any_io_executor executor() = 0;

        virtual void start() = 0;
        virtual void stop() = 0;
        virtual void read() = 0;
        virtual void write(IoBuffer event) = 0;
        virtual std::vector<std::uint8_t> udp_associate() = 0;

        [[nodiscard]] int id() const { return id_; }
        StreamManagerPtr manager() { return stream_manager_; }

    private:
        StreamManagerPtr stream_manager_;
        int id_;
    };

    using ServerStreamPtr = std::shared_ptr<ServerStream>;
}

#endif // MTLS_MPROXY_TRANSPORT_SERVER_STREAM_H
