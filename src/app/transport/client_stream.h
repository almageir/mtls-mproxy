#ifndef MTLS_MPROXY_TRANSPORT_CLIENT_STREAM_H
#define MTLS_MPROXY_TRANSPORT_CLIENT_STREAM_H

#include "stream.h"

#include <string>

namespace mtls_mproxy
{
    class ClientStream
        : public Stream
        , public std::enable_shared_from_this<ClientStream>
    {
    public:
        ClientStream(const StreamManagerPtr& smp, int id) : Stream(smp, id) {}

        void set_host(std::string host) { do_set_host(std::move(host)); }
        void set_service(std::string service) { do_set_service(std::move(service)); }

    private:
        virtual void do_set_host(std::string host) = 0;
        virtual void do_set_service(std::string service) = 0;
    };

    using ClientStreamPtr = std::shared_ptr<ClientStream>;
}

#endif // MTLS_MPROXY_TRANSPORT_CLIENT_STREAM_H
