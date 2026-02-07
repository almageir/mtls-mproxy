#ifndef MTLS_MPROXY_TRANSPORT_CLIENT_STREAM_H
#define MTLS_MPROXY_TRANSPORT_CLIENT_STREAM_H

#include "io_buffer.h"

#include <string>
#include <memory>

namespace mtls_mproxy
{
    class StreamManager;
    using StreamManagerPtr = std::shared_ptr<StreamManager>;

    class ClientStream
    {
    public:
        explicit ClientStream(StreamManagerPtr smp, int id = 0)
            : stream_manager_(std::move(smp))
            , id_(id)
        {}

        virtual ~ClientStream() = default;

        virtual void start() = 0;
        virtual void stop() = 0;
        virtual void read() = 0;
        virtual void write(IoBuffer event) = 0;

        virtual void set_host(std::string host) = 0;
        virtual void set_service(std::string service) = 0;

        [[nodiscard]] int id() const { return id_; }
        StreamManagerPtr manager() { return stream_manager_; }

    private:
        StreamManagerPtr stream_manager_;
        int id_;
    };

    using ClientStreamPtr = std::shared_ptr<ClientStream>;
}

#endif // MTLS_MPROXY_TRANSPORT_CLIENT_STREAM_H
