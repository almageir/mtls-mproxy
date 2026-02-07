#ifndef MTLS_MPROXY_FWD_SESSION_H
#define MTLS_MPROXY_FWD_SESSION_H

#include "fwd_state.h"

#include <asynclog/scoped_logger.h>

#include <string>

namespace asynclog {
	class LoggerFactory;
}

namespace mtls_mproxy
{
    class StreamManager;
    using StreamManagerPtr = std::shared_ptr<StreamManager>;

    class FwdSession
    {
        struct FwdCtx {
            int id;
            std::vector<uint8_t> response;
            std::string host;
            std::string service;
            std::size_t transferred_bytes_to_remote;
            std::size_t transferred_bytes_to_local;
        };

    public:
        FwdSession(int id, StreamManagerPtr manager, asynclog::LoggerFactory logger_factory);
        void change_state(std::unique_ptr<FwdState> state);
        void handle_server_read(IoBuffer& event);
        void handle_client_read(IoBuffer& event);
        void handle_server_write();
        void handle_client_write();
        void handle_client_connect(IoBuffer& event);
        void handle_on_accept();
        void handle_server_error(net::error_code ec);
        void handle_client_error(net::error_code ec);

        auto& context() { return context_; }
        const auto& context() const { return context_; }

        void update_bytes_sent_to_remote(std::size_t count);
        void update_bytes_sent_to_local(std::size_t count);

        int id() { return context().id; }
        std::string_view host() const { return context().host; }
        std::string_view service() const { return context().service; }
        std::uint64_t transferred_bytes_to_local() const { return context().transferred_bytes_to_local; }
        std::uint64_t transferred_bytes_to_remote() const { return context().transferred_bytes_to_remote; }

        const std::vector<uint8_t>& get_response() const { return context().response; }

        void set_endpoint_info(std::string_view host, std::string_view service) {
            context().host = host;
            context().service = service;
        }

        void connect();
        void stop();
        void read_from_server();
        void read_from_client();

        void write_to_client(IoBuffer buffer);
        void write_to_server(IoBuffer buffer);

        StreamManagerPtr manager();
        asynclog::ScopedLogger& logger() { return logger_; }

    private:
        FwdCtx context_;
        std::unique_ptr<FwdState> state_;
        StreamManagerPtr manager_;
        asynclog::ScopedLogger logger_;
    };
}

#endif // MTLS_MPROXY_FWD_SESSION_H
