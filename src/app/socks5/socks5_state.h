#ifndef SOCKS5_STATE_H
#define SOCKS5_STATE_H

#include "transport/io_buffer.h"

#include <asio.hpp>
#include <memory>

namespace net = asio;

class socks5_session;

class socks5_state
{
public:
    virtual ~socks5_state() = default;
    virtual void handle_server_read(socks5_session *session, io_buffer event);
    virtual void handle_client_read(socks5_session *session, io_buffer event);
    virtual void handle_client_connect(socks5_session *session, io_buffer event);
    virtual void handle_server_write(socks5_session *session, io_buffer event);
    virtual void handle_client_write(socks5_session *session, io_buffer event);
    virtual void handle_server_error(socks5_session* session, net::error_code ec);
    virtual void handle_client_error(socks5_session* session, net::error_code ec);
};

class socks5_auth_request final : public socks5_state
{
public:
    static auto instance() { return std::make_unique<socks5_auth_request>(); }
    void handle_server_read(socks5_session *session, io_buffer event) override;
};

class socks5_connection_request final : public socks5_state
{
public:
    static auto instance() { return std::make_unique<socks5_connection_request>(); }
    void handle_server_read(socks5_session *session, io_buffer event) override;
    void handle_server_write(socks5_session *session, io_buffer event) override;
};

class socks5_connection_established final : public socks5_state
{
public:
    static auto instance() { return std::make_unique<socks5_connection_established>(); }
    void handle_client_connect(socks5_session *session, io_buffer event) override;
    void handle_client_error(socks5_session* session, net::error_code ec) override;
    void handle_server_write(socks5_session* session, io_buffer event) override;
};

class socks5_ready_to_transfer_data final : public socks5_state
{
public:
    static auto instance() { return std::make_unique<socks5_ready_to_transfer_data>(); }
    void handle_server_write(socks5_session *session, io_buffer event) override;
};

class socks5_data_transfer_mode final : public socks5_state
{
public:
    static auto instance() { return std::make_unique<socks5_data_transfer_mode>(); }
    void handle_server_read(socks5_session *session, io_buffer event) override;
    void handle_server_write(socks5_session *session, io_buffer event) override;
    void handle_client_read(socks5_session *session, io_buffer event) override;
    void handle_client_write(socks5_session *session, io_buffer event) override;
};

#endif // SOCKS5_STATE_H
