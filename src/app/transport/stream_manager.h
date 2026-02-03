#ifndef MTLS_MPROXY_TRANSPORT_STREAM_MANAGER_H
#define MTLS_MPROXY_TRANSPORT_STREAM_MANAGER_H

#include "server_stream.h"
#include "client_stream.h"

namespace mtls_mproxy
{
    class StreamManager
        : public std::enable_shared_from_this<StreamManager>
    {
    public:
        virtual ~StreamManager() = default;
        // Common interface
        virtual void stop(stream_ptr ptr) = 0;
        virtual void stop(int id) = 0;
        virtual void on_close(stream_ptr stream) = 0;

        // Passive session interface
        virtual void on_accept(ServerStreamPtr ptr) = 0;
        virtual void on_read(IoBuffer event, ServerStreamPtr stream) = 0;
        virtual void on_write(IoBuffer event, ServerStreamPtr stream) = 0;
        virtual void on_error(net::error_code ec, ServerStreamPtr stream) = 0;
        virtual void read_server(int id) = 0;
        virtual void write_server(int id, IoBuffer event) = 0;
        virtual void on_server_ready(ServerStreamPtr ptr) = 0;

        // Active session interface
        virtual void on_connect(IoBuffer event, ClientStreamPtr stream) = 0;
        virtual void on_read(IoBuffer event, ClientStreamPtr stream) = 0;
        virtual void on_write(IoBuffer event, ClientStreamPtr stream) = 0;
        virtual void on_error(net::error_code ec, ClientStreamPtr stream) = 0;
        virtual void read_client(int id) = 0;
        virtual void write_client(int id, IoBuffer event) = 0;
        virtual void connect(int id, std::string host, std::string service) = 0;

        virtual std::vector<std::uint8_t> udp_associate(int id) = 0;
    };

    using StreamManagerPtr = std::shared_ptr<StreamManager>;
}

#endif // MTLS_MPROXY_TRANSPORT_STREAM_MANAGER_H
