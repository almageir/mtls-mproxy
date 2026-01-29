#ifndef MTLS_MPROXY_TRANSPORT_SERVER_STREAM_H
#define MTLS_MPROXY_TRANSPORT_SERVER_STREAM_H

#include "stream.h"

#include <asio/any_io_executor.hpp>

namespace mtls_mproxy
{
    namespace net = asio;

    class ServerStream
        : public Stream
        , public std::enable_shared_from_this<ServerStream>
    {
    public:
        ServerStream(const StreamManagerPtr& smp, int id) : Stream(smp, id) {}
        virtual net::any_io_executor executor() = 0;
    };

    using ServerStreamPtr = std::shared_ptr<ServerStream>;
}

#endif // MTLS_MPROXY_TRANSPORT_SERVER_STREAM_H
