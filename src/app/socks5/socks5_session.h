#ifndef socks5_session_h__
#define socks5_session_h__

#include "socks5.h"
#include "socks5_state.h"
#include "asynclog/scoped_logger.h"

namespace asynclog {
    class LoggerFactory;
}

class stream_manager;
using stream_manager_ptr = std::shared_ptr<stream_manager>;

class socks5_session
{
    struct socks_ctx {
        int id;
        std::vector<std::uint8_t> response;
        std::string host;
        std::string service;
        std::size_t transferred_bytes_to_remote;
        std::size_t transferred_bytes_to_local;

        socks5::request_header* request_hdr() {
            return reinterpret_cast<socks5::request_header*>(response.data());
        }
    };

public:
    socks5_session(int id, stream_manager_ptr manager, asynclog::LoggerFactory factory);

    void change_state(std::unique_ptr<socks5_state> state);
    void handle_server_read(io_buffer event);
    void handle_client_read(io_buffer event);
    void handle_server_write(io_buffer event);
    void handle_client_write(io_buffer event);
    void handle_client_connect(io_buffer event);
    void handle_server_error(net::error_code ec);
    void handle_client_error(net::error_code ec);

    void update_bytes_sent_to_remote(std::size_t count);
    void update_bytes_sent_to_local(std::size_t count);

    auto& context() { return context_; }
    const auto& context() const { return context_; }

    int id() { return context().id; }
    std::string_view host() const { return context().host; }
    std::string_view service() const { return context().service; }
    std::uint64_t transfered_bytes_to_local() const { return context().transferred_bytes_to_local; }
    std::uint64_t transfered_bytes_to_remote() const { return context().transferred_bytes_to_remote; }

    void set_response(std::uint8_t version, std::uint8_t auth_mode) {
        context().response.resize(2);
        context().response[0] = version;
        context().response[1] = auth_mode;
    }

    void set_endpoint_info(std::string_view host, std::string_view service) {
        context().host = host;
        context().service = service;
    }

    void set_response(io_buffer buffer) { context().response = std::move(buffer); }
    void set_response_error_code(std::uint8_t err_code) { context().request_hdr()->command = err_code; }

    void connect();
    void stop();
    void read_from_server();
    void read_from_client();

    void write_to_client(io_buffer buffer);
    void write_to_server(io_buffer buffer);

    std::vector<std::uint8_t> response() const { return context().response; }

   stream_manager_ptr manager();

    asynclog::ScopedLogger& logger();

private:
    socks_ctx context_;
    std::unique_ptr<socks5_state> state_;
    stream_manager_ptr manager_;
    asynclog::ScopedLogger logger_;
};

#endif // socks5_session_h__
