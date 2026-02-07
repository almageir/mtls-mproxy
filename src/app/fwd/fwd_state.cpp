#include "fwd_state.h"
#include "fwd_session.h"
#include "transport/stream_manager.h"

#include <format>

namespace
{
    namespace net = asio;

    std::string check_errors(mtls_mproxy::FwdSession& session, net::error_code ec, std::string_view participant)
    {
        const auto error = ec.value();
        const auto msg = ec.message();

        if (ec) {
            if (!(error == net::error::eof ||
                  error == net::error::connection_aborted ||
                  error == net::error::connection_refused ||
                  error == net::error::connection_reset ||
                  error == net::error::broken_pipe ||
                  error == net::error::timed_out ||
                  error == net::error::operation_aborted ||
                  error == net::error::bad_descriptor)) {
                  return std::format("[{}] {} side session error: {}", session.id(), participant, msg);
            }
        }

        return {};
    }
}

namespace mtls_mproxy
{
    void FwdState::handle_server_read(FwdSession& session, IoBuffer buffer) {}
    void FwdState::handle_client_read(FwdSession& session, IoBuffer buffer) {}
    void FwdState::handle_on_accept(FwdSession& session) {}
    void FwdState::handle_client_connect(FwdSession& session, IoBuffer buffer) {}
    void FwdState::handle_server_write(FwdSession& session) {}
    void FwdState::handle_client_write(FwdSession& session) {}

    void FwdState::handle_server_error(FwdSession& session, net::error_code ec)
    {
        const auto errors = check_errors(session, ec, "server");
        if (!errors.empty())
            session.logger().warn(errors);
        session.stop();
    }

    void FwdState::handle_client_error(FwdSession& session, net::error_code ec)
    {
        const auto errors = check_errors(session, ec, "client");
        if (!errors.empty())
            session.logger().warn(errors);
        session.stop();
    }

    void FwdWaitConnection::handle_on_accept(FwdSession& session)
    {
        const auto sid = session.id();
        session.logger().info(std::format("[{}] requested [{}:{}]", sid, session.host(), session.service()));
        session.connect(); // ��� ������ ���� ����� ������� ������ ������ �� ��������� ������
        session.change_state(FwdConnectionEstablished::instance());
    }

    void FwdConnectionEstablished::handle_client_connect(FwdSession& session, IoBuffer buffer)
    {
        session.read_from_server();
        session.read_from_client();
        session.change_state(FwdDataTransferMode::instance());
    }

    void FwdConnectionEstablished::handle_server_read(FwdSession& session, IoBuffer buffer)
    {
        session.update_bytes_sent_to_remote(buffer.size());
        session.write_to_client(std::move(buffer));
        session.change_state(FwdDataTransferMode::instance());
    }

    void FwdDataTransferMode::handle_server_write(FwdSession& session)
    {
        session.read_from_client();
    }

    void FwdDataTransferMode::handle_server_read(FwdSession& session, IoBuffer buffer)
    {
        session.update_bytes_sent_to_remote(buffer.size());
        session.write_to_client(std::move(buffer));
    }

    void FwdDataTransferMode::handle_client_write(FwdSession& session)
    {
        session.read_from_server();
    }

    void FwdDataTransferMode::handle_client_read(FwdSession& session, IoBuffer buffer)
    {
        session.update_bytes_sent_to_local(buffer.size());
        session.write_to_server(std::move(buffer));
    }
}