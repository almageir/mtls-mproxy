#include "socks.h"
#include "socks_state.h"
#include "socks_session.h"
#include "transport/stream_manager.h"

#include <asynclog/scoped_logger.h>

#include <format>

namespace
{
    std::uint8_t get_response_error_code(net::error_code ec)
    {
        const auto err_val = ec.value();

        if (err_val == net::error::network_unreachable)
            return Socks::Responses::network_unreachable;

        if (err_val == net::error::host_unreachable ||
                   err_val == net::error::host_not_found ||
                   err_val == net::error::no_data)
            return Socks::Responses::host_unreachable;

        if (err_val == net::error::connection_refused ||
                   err_val == net::error::connection_aborted ||
                   err_val == net::error::connection_reset)
            return Socks::Responses::connection_refused;

        if (err_val == net::error::timed_out)
            return Socks::Responses::ttl_expired;

        return Socks::Responses::general_socks_server_failure;
    }

    std::string check_errors(mtls_mproxy::SocksSession& session, net::error_code ec, std::string_view participant)
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
                  return std::format("[{}] {} side session error: {}", session.id(),participant, msg);
            }
        }

        return {};
    }
}

namespace mtls_mproxy
{
    void SocksState::handle_server_read(SocksSession& session, IoBuffer buffer) {}
    void SocksState::handle_client_read(SocksSession& session, IoBuffer buffer) {}
    void SocksState::handle_client_connect(SocksSession& session, IoBuffer buffer) {}
    void SocksState::handle_server_write(SocksSession& session, IoBuffer buffer) {}
    void SocksState::handle_client_write(SocksSession& session, IoBuffer buffer) {}

    void SocksState::handle_server_error(SocksSession& session, net::error_code ec)
    {
        const auto errors = check_errors(session, ec, "server");
        if (!errors.empty())
            session.logger().warn(errors);
        session.stop();
    }

    void SocksState::handle_client_error(SocksSession& session, net::error_code ec)
    {
        const auto errors = check_errors(session, ec, "client");
        if (!errors.empty())
            session.logger().warn(errors);
        session.stop();
    }

    void SocksAuthRequest::handle_server_read(SocksSession& session, IoBuffer buffer) {
        const auto error = Socks::is_socks5_auth_request(buffer.data(), buffer.size());
        const auto auth_mode = error ? proto::AuthMethod::NotSupported : proto::AuthMethod::NoAuth;

        session.set_response(proto::version, auth_mode);
        if (auth_mode == proto::AuthMethod::NotSupported)
            session.logger().warn(std::format("[{}] {}", session.id(), error.value_or("")));

        session.write_to_server(std::move(IoBuffer{session.response()}));
        session.change_state(SocksConnectionRequest::instance());
    }

    void SocksConnectionRequest::handle_server_write(SocksSession& session, IoBuffer buffer) {
        session.manager()->read_server(session.id());
    }

    void SocksConnectionRequest::handle_server_read(SocksSession& session, IoBuffer buffer) {
        const auto sid = session.id();

        if (!Socks::is_valid_request_packet(buffer.data(), buffer.size())) {
            session.logger().warn(std::format("[{}] socks5 protocol: bad request packet", sid));
            session.stop();
            return;
        }

        std::string host, service;
        if (!Socks::get_remote_address_info(buffer.data(), buffer.size(), host, service)) {
            session.logger().warn(std::format("[{}] socks5 protocol: bad remote address format", sid));
            session.stop();
            return;
        }

        session.set_endpoint_info(host, service);

        session.logger().info(std::format("[{}] requested [{}:{}]", sid, host, service));
        session.set_response(std::move(buffer));
        session.connect();
        session.change_state(SocksConnectionEstablished::instance());
    }

    void SocksConnectionEstablished::handle_client_connect(SocksSession& session, IoBuffer buffer) {
        session.set_response_error_code(Socks::Responses::succeeded);
        session.write_to_server(std::move(IoBuffer{session.response()}));
        session.change_state(SocksReadyTransferData::instance());
    }

    void SocksConnectionEstablished::handle_client_error(SocksSession& session, net::error_code ec)
    {
        session.set_response_error_code(get_response_error_code(ec));
        session.write_to_server(std::move(IoBuffer{session.response()}));
        session.logger().warn(std::format("[{}] client side session error: {}", session.id(), ec.message()));
    }

    void SocksConnectionEstablished::handle_server_write(SocksSession& session, IoBuffer buffer)
    {
        session.stop();
    }

    void SocksReadyTransferData::handle_server_write(SocksSession& session, IoBuffer buffer) {
        session.read_from_server();
        session.read_from_client();
        session.change_state(SocksDataTransferMode::instance());
    }

    void SocksDataTransferMode::handle_server_write(SocksSession& session, IoBuffer buffer) {
        session.read_from_client();
    }

    void SocksDataTransferMode::handle_server_read(SocksSession& session, IoBuffer buffer) {
        session.update_bytes_sent_to_remote(buffer.size());
        session.write_to_client(std::move(buffer));
    }

    void SocksDataTransferMode::handle_client_write(SocksSession& session, IoBuffer buffer) {
        session.read_from_server();
    }

    void SocksDataTransferMode::handle_client_read(SocksSession& session, IoBuffer buffer) {
        session.update_bytes_sent_to_local(buffer.size());
        session.write_to_server(std::move(buffer));
    }
}
