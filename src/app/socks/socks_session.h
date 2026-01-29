#ifndef MTLS_MPROXY_SOCKS_SESSION_H
#define MTLS_MPROXY_SOCKS_SESSION_H

#include "socks.h"
#include "socks_state.h"

#include <asynclog/scoped_logger.h>

#include <cstdint>
#include <memory>

namespace asynclog {
    class LoggerFactory;
}

namespace mtls_mproxy
{
    class StreamManager;
    using StreamManagerPtr = std::shared_ptr<StreamManager>;

    class SocksSession
    {
        struct SocksCtx {
            int id;
            std::vector<std::uint8_t> response;
            std::string host;
            std::string service;
            std::size_t transferred_bytes_to_remote;
            std::size_t transferred_bytes_to_local;

            Socks::RequestHeader* request_hdr() {
                return reinterpret_cast<Socks::RequestHeader*>(response.data());
            }
        };

    public:
        SocksSession(int id, StreamManagerPtr manager, const asynclog::LoggerFactory& logger_factory);

        void change_state(std::unique_ptr<SocksState> state);
        void handle_server_read(IoBuffer event);
        void handle_client_read(IoBuffer event);
        void handle_server_write(IoBuffer event);
        void handle_client_write(IoBuffer event);
        void handle_client_connect(IoBuffer event);
        void handle_server_error(net::error_code ec);
        void handle_client_error(net::error_code ec);

        void update_bytes_sent_to_remote(std::size_t count);
        void update_bytes_sent_to_local(std::size_t count);

        auto& context() { return context_; }
        const auto& context() const { return context_; }

        int id() { return context().id; }
        std::string_view host() const { return context().host; }
        std::string_view service() const { return context().service; }
        std::uint64_t transferred_bytes_to_local() const { return context().transferred_bytes_to_local; }
        std::uint64_t transferred_bytes_to_remote() const { return context().transferred_bytes_to_remote; }

        void set_response(std::uint8_t version, std::uint8_t auth_mode) {
            context().response.resize(2);
            context().response[0] = version;
            context().response[1] = auth_mode;
        }

        void set_endpoint_info(std::string_view host, std::string_view service) {
            context().host = host;
            context().service = service;
        }

        void set_response(IoBuffer buffer) { context().response = std::move(buffer); }
        void set_response_error_code(std::uint8_t err_code) { context().request_hdr()->command = err_code; }

        void connect();
        void stop();
        void read_from_server();
        void read_from_client();

        void write_to_client(IoBuffer buffer);
        void write_to_server(IoBuffer buffer);

        std::vector<std::uint8_t> response() const { return context().response; }

        StreamManagerPtr manager();

        asynclog::ScopedLogger& logger();

    private:
        SocksCtx context_;
        std::unique_ptr<SocksState> state_;
        StreamManagerPtr manager_;
        asynclog::ScopedLogger logger_;
    };
}

#endif // MTLS_MPROXY_SOCKS_SESSION_H
