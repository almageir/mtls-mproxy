#ifndef HTTP_SESSION_H
#define HTTP_SESSION_H

#include "http_state.h"

#include <asynclog/logger_factory.h>

#include <string>

class stream_manager;
using stream_manager_ptr = std::shared_ptr<stream_manager>;

class http_session
{
    struct http_ctx {
        int id;
        std::vector<uint8_t> response;
        std::string host;
        std::string service;
        std::size_t transferred_bytes_to_remote;
        std::size_t transferred_bytes_to_local;
    };

public:
    http_session(int id, stream_manager_ptr manager, asynclog::LoggerFactory log_factory);
    void change_state(std::unique_ptr<http_state> state);
    void handle_server_read(io_buffer& event);
    void handle_client_read(io_buffer& event);
    void handle_server_write(io_buffer& event);
    void handle_client_write(io_buffer& event);
    void handle_client_connect(io_buffer& event);
    void handle_server_error(net::error_code ec);
    void handle_client_error(net::error_code ec);

    auto& context() { return context_; }
    const auto& context() const { return context_; }

	void update_bytes_sent_to_remote(std::size_t count);
	void update_bytes_sent_to_local(std::size_t count);

	int id() { return context().id; }
	std::string_view host() const { return context().host; }
	std::string_view service() const { return context().service; }
	std::uint64_t transfered_bytes_to_local() const { return context().transferred_bytes_to_local; }
	std::uint64_t transfered_bytes_to_remote() const { return context().transferred_bytes_to_remote; }

    const std::vector<uint8_t>& get_response() const { return context().response; }

	void set_endpoint_info(std::string_view host, std::string_view service) {
		context().host = host;
		context().service = service;
	}

	void set_response(io_buffer buffer) { context().response = std::move(buffer); }

	void connect();
	void stop();
	void read_from_server();
	void read_from_client();

	void write_to_client(io_buffer buffer);
	void write_to_server(io_buffer buffer);

    stream_manager_ptr manager();
	asynclog::ScopedLogger& logger() { return logger_; }

private:
    http_ctx context_;
    std::unique_ptr<http_state> state_;
    stream_manager_ptr manager_;
	asynclog::LoggerFactory log_factory_;
	asynclog::ScopedLogger logger_;
};

#endif //HTTP_SESSION_H
