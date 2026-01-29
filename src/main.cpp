#include "transport/tcp/server.h"
#include "transport/tls/tls_server.h"
#include "socks/socks_stream_manager.h"
#include "http/http_stream_manager.h"

#include <asynclog/log_manager.h>
#include <asynclog/scoped_logger.h>
#include <asynclog/logger_factory.h>

#include <cliap/cliap.h>

#include <optional>

namespace
{
    struct server_conf
    {
        std::string listen_port;
        std::string proxy_backend;
        std::string log_file_path;
        int log_level;
        mtls_mproxy::TlsServer::TlsOptions tls_options;
    };

    std::optional<server_conf> parse_command_line_arguments(int argc, char* argv[])
    {
        server_conf srv_conf{};

        using namespace cliap;

        ArgParser argParser;

        argParser
            .add_parameter(Arg("h,help").flag().description("show help message"))
            .add_parameter(Arg("p,port").required().set_default("8443").description("proxy server listen port number"))
            .add_parameter(Arg("m,mode").required().set_default("http").description("proxy mode [http|socks5]"))
            .add_parameter(Arg("v,log_level").set_default("info").description("verbosity level of log messages [debug|trace|info|warning|error|fatal]"))
            .add_parameter(Arg("l,log_file").description("log file path"))
            .add_parameter(Arg("t,tls").flag().description("use tls tunnel mode"))
            .add_parameter(Arg("k,private-key").set_default("").description("private key file path"))
            .add_parameter(Arg("s,server-cert").set_default("").description("server certificate file path"))
            .add_parameter(Arg("c,ca-cert").set_default("").description("CA certificate file path"));

        const auto err_msg = argParser.parse(argc, argv);
        if (argParser.arg("h").is_parsed()) {
            argParser.print_help();
            return std::nullopt;
        }

        if (err_msg.has_value()) {
            std::cout << *err_msg << std::endl;
            argParser.print_help();
            return std::nullopt;
        }

        srv_conf.listen_port = argParser.arg("p").get_value_as_str();
        srv_conf.proxy_backend = argParser.arg("m").get_value_as_str();
        // srv_conf.log_level = parse_log_level(argParser.arg("v").get_value_as_str());
        srv_conf.log_file_path = argParser.arg("l").get_value_as_str();

        if (argParser.arg("t").is_parsed()) {
            srv_conf.tls_options.private_key = argParser.arg("k").get_value_as_str();
            if (srv_conf.tls_options.private_key.empty()) {
                std::cerr << "In tls mode, the <private-key> parameter must be specified\n";
                return std::nullopt;
            }
            srv_conf.tls_options.server_cert = argParser.arg("s").get_value_as_str();
            if (srv_conf.tls_options.server_cert.empty()) {
                std::cerr << "In tls mode, the <server-cert> parameter must be specified\n";
                return std::nullopt;
            }
            srv_conf.tls_options.ca_cert = argParser.arg("c").get_value_as_str();
            if (srv_conf.tls_options.ca_cert.empty()) {
                std::cerr << "In tls mode, the <ca-cert> parameter must be specified\n";
                return std::nullopt;
            }
        }

        return srv_conf;
    }
}

int main(int argc, char* argv[])
{
#ifdef _WIN32
    std::locale::global(std::locale(""));
#endif

    const auto serv_conf = parse_command_line_arguments(argc, argv);
    if (!serv_conf.has_value()) {
        std::cerr << "Program finished..." << std::endl;
        return 0;
    }
    const server_conf& conf = serv_conf.value();

    namespace asl = asynclog;
    const auto log_backend = std::make_shared<asl::LogManager>();
    log_backend->open(asl::LogMode::Console | asl::LogMode::File, conf.log_file_path);

    asl::LoggerFactory log_factory(log_backend);
    const auto logger = log_factory.create("Application");

    using namespace mtls_mproxy;

    try {
        StreamManagerPtr proxy_backend;
        if (conf.proxy_backend == "http") {
            logger.info("Proxy-mode: http/s");
            proxy_backend = std::make_shared<HttpStreamManager>(log_factory);
        } else {
            logger.info("Proxy-mode: socks5/s");
            proxy_backend = std::make_shared<SocksStreamManager>(log_factory);
        }

        if (!conf.tls_options.private_key.empty()) {
            logger.info(std::format("Start listening on port: {}, tls tunnel mode enabled", conf.listen_port));
            TlsServer srv(conf.listen_port, conf.tls_options, std::move(proxy_backend), log_factory);
            srv.run();
        } else {
            logger.info(std::format("Start listening on port: {}, tls tunnel mode disabled", conf.listen_port));
            server srv(conf.listen_port, std::move(proxy_backend), log_factory);
            srv.run();
        }
    } catch (std::exception& ex) {
        std::cerr << ex.what() << std::endl;
        logger.info(std::format("fatal error: {}", ex.what()));
    }

    return 0;
}