#ifndef SOCKS5_STREAM_MANAGER_H
#define SOCKS5_STREAM_MANAGER_H

#include "transport/stream_manager.h"
#include "socks5_session.h"
#include "asynclog/logger_factory.h"

class socks5_stream_manager final
    : public stream_manager
{
public:
    socks5_stream_manager(asynclog::LoggerFactory log_factory);
    ~socks5_stream_manager() = default;

    socks5_stream_manager(const socks5_stream_manager& other) = delete;
    socks5_stream_manager& operator=(const socks5_stream_manager& other) = delete;

    void stop(stream_ptr stream) override;
    void stop(int id) override;
    void on_close(stream_ptr stream) override;

    void on_accept(server_stream_ptr stream) override;
    void on_read(io_buffer buffer, server_stream_ptr stream) override;
    void on_write(io_buffer buffer, server_stream_ptr stream) override;
    void on_error(net::error_code ec, server_stream_ptr stream) override;
    void read_server(int id) override;
    void write_server(int id, io_buffer buffer) override;

    void on_connect(io_buffer buffer, client_stream_ptr stream) override;
    void on_read(io_buffer buffer, client_stream_ptr stream) override;
    void on_write(io_buffer buffer, client_stream_ptr stream) override;
    void on_error(net::error_code ec, client_stream_ptr stream) override;
    void read_client(int id) override;
    void write_client(int id, io_buffer buffer) override;
    void connect(int id, std::string host, std::string service) override;

private:
    struct socks_pair {
        int id;
        server_stream_ptr server;
        client_stream_ptr client;
        socks5_session session;
    };

    std::unordered_map<int, socks_pair> sessions_;
    asynclog::LoggerFactory logger_factory_;
    asynclog::ScopedLogger logger_;
};

using socks5_stream_manager_ptr = std::shared_ptr<socks5_stream_manager>;


#endif // SOCKS5_STREAM_MANAGER_H
