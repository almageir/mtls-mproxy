#ifndef STREAM_MANAGER_H
#define STREAM_MANAGER_H

#include "server_stream.h"
#include "client_stream.h"

class stream_manager
    : public std::enable_shared_from_this<stream_manager>
{
public:
    virtual ~stream_manager() = default;
    // Common interface
    virtual void stop(stream_ptr ptr) = 0;
    virtual void stop(int id) = 0;
    virtual void on_close(stream_ptr stream) = 0;

    // Passive session interface
    virtual void on_accept(server_stream_ptr ptr) = 0;
    virtual void on_read(io_buffer event, server_stream_ptr stream) = 0;
    virtual void on_write(io_buffer event, server_stream_ptr stream) = 0;
    virtual void on_error(net::error_code ec, server_stream_ptr stream) = 0;
    virtual void read_server(int id) = 0;
    virtual void write_server(int id, io_buffer event) = 0;

    // Active session interface
    virtual void on_connect(io_buffer event, client_stream_ptr stream) = 0;
    virtual void on_read(io_buffer event, client_stream_ptr stream) = 0;
    virtual void on_write(io_buffer event, client_stream_ptr stream) = 0;
    virtual void on_error(net::error_code ec, client_stream_ptr stream) = 0;
    virtual void read_client(int id) = 0;
    virtual void write_client(int id, io_buffer event) = 0;
    virtual void connect(int id, std::string host, std::string service) = 0;
};

using stream_manager_ptr = std::shared_ptr<stream_manager>;


#endif // STREAM_MANAGER_H
