#include "http.h"
#include "http_state.h"
#include "http_session.h"

#include <asio/error.hpp>

#include <format>

namespace
{
    namespace net = asio;

    constexpr std::string_view kHttpError500 =
        "HTTP/1.1 500 Internal Server Error\r\n"
        "Connection : Closed\r\n"
        "\r\n";

    constexpr std::string_view kHttpDone =
        "HTTP/1.1 200 OK\r\n"
        "\r\n";

    std::string check_errors(mtls_mproxy::HttpSession& session, net::error_code ec, std::string_view participant)
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
                  return std::format("[{}] {} side session error: {}", session.id(),participant, msg);
            }
        }

        return {};
    }
}

namespace mtls_mproxy
{
    void HttpState::handle_server_read(HttpSession& session, IoBuffer buffer) {}
    void HttpState::handle_client_read(HttpSession& session, IoBuffer buffer) {}
    void HttpState::handle_on_accept(HttpSession& session) {}
    void HttpState::handle_client_connect(HttpSession& session, IoBuffer buffer) {}
    void HttpState::handle_server_write(HttpSession& session) {}
    void HttpState::handle_client_write(HttpSession& session) {}

    void HttpState::handle_server_error(HttpSession& session, net::error_code ec)
    {
        const auto errors = check_errors(session, ec, "server");
        if (!errors.empty())
            session.logger().warn(errors);
        session.stop();
    }

    void HttpState::handle_client_error(HttpSession& session, net::error_code ec)
    {
        const auto errors = check_errors(session, ec, "client");
        if (!errors.empty())
            session.logger().warn(errors);
        session.stop();
    }

    void HttpWaitRequest::handle_on_accept(HttpSession& session)
    {
        session.read_from_server();
    }

    void HttpWaitRequest::handle_server_read(HttpSession& session, IoBuffer buffer)
    {
        const auto sid = session.id();
        const auto* req_str = reinterpret_cast<const char*>(buffer.data());

        auto http_req = http::get_headers(std::string_view{req_str, buffer.size()});
        const auto host = http_req.get_host();
        const auto service = http_req.get_service();

        if (host.empty()) {
            session.logger().warn(std::format("[{}] http protocol: bad request packet", sid));
            session.write_to_server(IoBuffer(kHttpError500.begin(), kHttpError500.end()));
            session.stop();
            return;
        }

        if (service.empty()) {
            session.logger().warn(std::format("[{}] http protocol: bad remote address format", sid));
            session.write_to_server(IoBuffer(kHttpError500.begin(), kHttpError500.end()));
            session.stop();
            return;
        }

        if (http_req.method == http::kConnect)
            session.set_response(IoBuffer(kHttpDone.begin(), kHttpDone.end()));
        else
            session.set_response(std::move(buffer));

        session.set_endpoint_info(host, service);

        session.logger().info(std::format("[{}] requested [{}:{}]", sid, host, service));
        session.connect();
        session.change_state(HttpConnectionEstablished::instance());
    }

    void HttpConnectionEstablished::handle_client_connect(HttpSession& session, IoBuffer buffer)
    {
        // TODO
        const std::string resp{session.get_response().begin(), session.get_response().end()};
        if (resp == kHttpDone)
            session.write_to_server(session.get_response());
        else
            session.write_to_client(session.get_response());
        session.change_state(HttpReadyTransferData::instance());
    }

    void HttpReadyTransferData::handle_client_write(HttpSession& session)
    {
        session.read_from_server();
        session.read_from_client();
        session.change_state(HttpDataTransferMode::instance());
    }

    void HttpReadyTransferData::handle_server_write(HttpSession& session)
    {
        session.read_from_server();
        session.read_from_client();
        session.change_state(HttpDataTransferMode::instance());
    }


    void HttpDataTransferMode::handle_server_write(HttpSession& session)
    {
        session.read_from_client();
    }

    void HttpDataTransferMode::handle_server_read(HttpSession& session, IoBuffer buffer)
    {
        session.update_bytes_sent_to_remote(buffer.size());
        session.write_to_client(std::move(buffer));
    }

    void HttpDataTransferMode::handle_client_write(HttpSession& session)
    {
        session.read_from_server();
    }

    void HttpDataTransferMode::handle_client_read(HttpSession& session, IoBuffer buffer)
    {
        session.update_bytes_sent_to_local(buffer.size());
        session.write_to_server(std::move(buffer));
    }
}