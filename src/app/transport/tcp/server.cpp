#include "server.h"
#include "tcp_server_stream.h"

#include <charconv>
#include <memory>

namespace mtls_mproxy
{
    Server::Server(const std::string& port, StreamManagerPtr proxy_backend, asynclog::LoggerFactory logger_factory)
        : signals_(ctx_)
        , acceptor_(ctx_)
        , stream_manager_(proxy_backend)
        , logger_factory_{logger_factory}
        , logger_{logger_factory.create("tcp_server")}
        , stream_id_(0)
    {
        configure_signals();
        async_wait_signals();

        std::uint16_t listen_port{0};
        std::from_chars(port.data(), port.data() + port.size(), listen_port);

        tcp::endpoint ep{tcp::endpoint(tcp::v4(), listen_port)};
        acceptor_.open(ep.protocol());
        acceptor_.set_option(tcp::acceptor::reuse_address(true));
        acceptor_.bind(ep);
        acceptor_.listen();

        logger_.info("proxy server starts on port: " + port);
        start_accept();
    }

    void Server::run()
    {
        ctx_.run();
    }

    void Server::configure_signals()
    {
        signals_.add(SIGINT);
        signals_.add(SIGTERM);
    }

    void Server::async_wait_signals()
    {
        signals_.async_wait(
            [this](net::error_code /*ec*/, int /*signno*/) {
            logger_.info("proxy server stopping");
            acceptor_.close();
            ctx_.stop();
            logger_.info("proxy server stopped");
        });
    }

    void Server::start_accept()
    {
        tcp::socket socket{ctx_.get_executor()};
        acceptor_.async_accept(
            [this](const net::error_code& ec, tcp::socket socket) {
                if (!acceptor_.is_open()) {
                    logger_.debug("proxy server acceptor is closed");
                    if (ec)
                        logger_.debug("proxy server error: " + ec.message());

                    return;
                }

                if (!ec) {
                    auto new_stream = std::make_shared<TcpServerStream>(
                        stream_manager_,
                        ++stream_id_,
                        std::move(socket),
                        logger_factory_);
                    stream_manager_->on_accept(std::move(new_stream));
                }

                start_accept();
            });
    }

    Server::~Server()
    {
        logger_.debug("proxy server stopped");
    }
}
