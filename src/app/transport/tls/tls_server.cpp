#include "tls_server.h"
#include "tls_server_stream.h"

#include <charconv>
#include <memory>

namespace mtls_mproxy
{
    TlsServer::TlsServer(const std::string& port,
                         const TlsOptions& settings,
                         StreamManagerPtr proxy_backend,
                         asynclog::LoggerFactory log_factory)
        : ssl_ctx_{net::ssl::context::tlsv13_server}
        , signals_(ctx_)
        , acceptor_(ctx_)
        , stream_manager_{std::move(proxy_backend)}
        , logger_factory_{std::move(log_factory)}
        , logger_{logger_factory_.create("tls_server")}
        , stream_id_(0)
    {
        configure_signals();
        async_wait_signals();

        ssl_ctx_.set_options(net::ssl::context::default_workarounds |
                             net::ssl::context::no_tlsv1_1 |
                             net::ssl::context::no_tlsv1_2);

        const auto rc = SSL_CTX_set_min_proto_version(ssl_ctx_.native_handle(), TLS1_3_VERSION);

        ssl_ctx_.use_certificate_chain_file(settings.server_cert);
        SSL_CTX_set_client_CA_list(ssl_ctx_.native_handle(), SSL_load_client_CA_file(settings.ca_cert.c_str()));
        ssl_ctx_.use_private_key_file(settings.private_key, net::ssl::context::pem);
        ssl_ctx_.load_verify_file(settings.ca_cert);
        ssl_ctx_.set_verify_mode(net::ssl::verify_peer | net::ssl::verify_fail_if_no_peer_cert);

        uint16_t listen_port{0};
        std::from_chars(port.data(), port.data() + port.size(), listen_port);

        tcp::endpoint ep{tcp::endpoint(tcp::v4(), listen_port)};
        acceptor_.open(ep.protocol());
        acceptor_.set_option(tcp::acceptor::reuse_address(true));
        acceptor_.bind(ep);
        acceptor_.listen();

        logger_.info("socks5-proxy tls_server starts on port: " + port);
        start_accept();
    }

    void TlsServer::run()
    {
        ctx_.run();
    }

    void TlsServer::configure_signals()
    {
        signals_.add(SIGINT);
        signals_.add(SIGTERM);
    }

    void TlsServer::async_wait_signals()
    {
        signals_.async_wait(
            [this](net::error_code /*ec*/, int /*signno*/) {
                logger_.info("socks5-proxy tls_server stopping");
                acceptor_.close();
                ctx_.stop();
                logger_.info("socks5-proxy tls_server stopped");
            });
    }

    void TlsServer::start_accept()
    {
        tcp::socket socket{ctx_.get_executor()};
        acceptor_.async_accept(
        [this](const net::error_code& ec, tcp::socket socket) {
                if (!acceptor_.is_open()) {
                    logger_.debug("tls proxy server acceptor is closed");

                    if (ec)
                        logger_.debug("tls proxy server error: " + ec.message());

                    return;
                }

                if (!ec) {
                    auto new_stream = std::make_shared<TlsServerStream>(
                        stream_manager_,
                        ++stream_id_,
                        ssl_socket{std::move(socket), ssl_ctx_},
                        logger_factory_);
                    stream_manager_->on_accept(std::move(new_stream));
                }

                start_accept();
            });
    }

    TlsServer::~TlsServer()
    {
        logger_.debug("tls proxy server stopped");
    }
}