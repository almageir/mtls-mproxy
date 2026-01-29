#include "socks5.h"
#include "socks5_state.h"
#include "socks5_session.h"
#include "transport/stream_manager.h"

#include <asynclog/scoped_logger.h>

namespace
{
    std::uint8_t get_response_error_code(net::error_code ec)
    {
        const auto errval = ec.value();

        if (errval == net::error::network_unreachable)
            return socks5::responses::network_unreachable;

        if (errval == net::error::host_unreachable ||
                   errval == net::error::host_not_found ||
                   errval == net::error::no_data)
            return socks5::responses::host_unreachable;

        if (errval == net::error::connection_refused ||
                   errval == net::error::connection_aborted ||
                   errval == net::error::connection_reset)
            return socks5::responses::connection_refused;

        if (errval == net::error::timed_out)
            return socks5::responses::ttl_expired;

        return socks5::responses::general_socks_server_failure;
    }

    std::string log_error(socks5_session* session, net::error_code ec, std::string_view participant)
    {
        const auto error = ec.value();
        const auto msg = ec.message();

        if (ec) {
            if (!(error == net::error::eof ||
                  error == net::error::connection_aborted ||
                  error == net::error::connection_refused ||
                  error == net::error::connection_reset ||
                  error == net::error::timed_out ||
                  error == net::error::operation_aborted ||
                  error == net::error::bad_descriptor)) {
                  return std::format("[{}] {} side session error: {}", session->id(),participant, msg);
            }
        }

        return {};
    }
}

void socks5_state::handle_server_read(socks5_session *session, io_buffer buffer) {}
void socks5_state::handle_client_read(socks5_session *session, io_buffer buffer) {}
void socks5_state::handle_client_connect(socks5_session *session, io_buffer buffer) {}
void socks5_state::handle_server_write(socks5_session *session, io_buffer buffer) {}
void socks5_state::handle_client_write(socks5_session *session, io_buffer buffer) {}

void socks5_state::handle_server_error(socks5_session* session, net::error_code ec)
{
    auto error = log_error(session, ec, "server");
    session->logger().warn(error);
    session->stop();
}

void socks5_state::handle_client_error(socks5_session* session, net::error_code ec)
{
    auto error = log_error(session, ec, "client");
    session->logger().warn(error);
    session->stop();
}

void socks5_auth_request::handle_server_read(socks5_session *session, io_buffer buffer) {
    const auto error = socks5::is_socks5_auth_request(buffer.data(), buffer.size());
    const auto auth_mode = error ? proto::auth::kNotSupported : proto::auth::kNoAuth;

    session->set_response(socks5::proto::version, auth_mode);
    if (auth_mode == proto::auth::kNotSupported)
        session->logger().warn(std::format("[{}] {}", session->id(), error.value_or("")));

    session->write_to_server(std::move(io_buffer{session->response()}));
    session->change_state(socks5_connection_request::instance());
}

void socks5_connection_request::handle_server_write(socks5_session *session, io_buffer buffer) {
    session->manager()->read_server(session->id());
}

void socks5_connection_request::handle_server_read(socks5_session *session, io_buffer buffer) {
    const auto sid = session->id();

    if (!socks5::is_valid_request_packet(buffer.data(), buffer.size())) {
        session->logger().warn(std::format("[{}] socks5 protocol: bad request packet", sid));
        session->stop();
        return;
    }

    std::string host, service;
    if (!socks5::get_remote_address_info(buffer.data(), buffer.size(), host, service)) {
        session->logger().warn(std::format("[{}] socks5 protocol: bad remote address format", sid));
        session->stop();
        return;
    }

    session->set_endpoint_info(host, service);

    session->logger().info(std::format("[{}] requested [{}:{}]", sid, host, service));
    session->set_response(std::move(buffer));
    session->connect();
    session->change_state(socks5_connection_established::instance());
}

void socks5_connection_established::handle_client_connect(socks5_session *session, io_buffer buffer) {
    session->set_response_error_code(socks5::responses::succeeded);
    session->write_to_server(std::move(io_buffer{session->response()}));
    session->change_state(socks5_ready_to_transfer_data::instance());
}

void socks5_connection_established::handle_client_error(socks5_session* session, net::error_code ec)
{
    session->set_response_error_code(get_response_error_code(ec));
    session->write_to_server(std::move(io_buffer{session->response()}));
    session->logger().warn(std::format("[{}] client side session error: {}", session->id(), ec.message()));
}

void socks5_connection_established::handle_server_write(socks5_session* session, io_buffer buffer)
{
    session->stop();
}

void socks5_ready_to_transfer_data::handle_server_write(socks5_session *session, io_buffer buffer) {
    session->read_from_server();
    session->read_from_client();
    session->change_state(socks5_data_transfer_mode::instance());
}

void socks5_data_transfer_mode::handle_server_write(socks5_session *session, io_buffer buffer) {
    session->read_from_client();
}

void socks5_data_transfer_mode::handle_server_read(socks5_session *session, io_buffer buffer) {
    session->update_bytes_sent_to_remote(buffer.size());
    session->write_to_client(std::move(buffer));
}

void socks5_data_transfer_mode::handle_client_write(socks5_session *session, io_buffer buffer) {
    session->read_from_server();
}

void socks5_data_transfer_mode::handle_client_read(socks5_session *session, io_buffer buffer) {
    session->update_bytes_sent_to_local(buffer.size());
    session->write_to_server(std::move(buffer));
}
