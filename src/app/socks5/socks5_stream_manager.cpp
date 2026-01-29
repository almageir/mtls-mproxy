#include "socks5_stream_manager.h"
#include "transport/tcp_client_stream.h"

socks5_stream_manager::socks5_stream_manager(asynclog::LoggerFactory log_factory)
    : logger_factory_{log_factory}
    , logger_{logger_factory_.create("socks5_session_manager")}
{
}

void socks5_stream_manager::stop(stream_ptr stream)
{
    stop(stream->id());
}

void socks5_stream_manager::stop(int id)
{
    if (auto it = sessions_.find(id); it != sessions_.end()) {
        it->second.client->stop();
        it->second.server->stop();

        const auto& ses = it->second.session;

        logger_.info(
            std::format("[{}] session closed: [{}:{}] rx_bytes: {}, tx_bytes: {}, live sessions {}",
                id,
                ses.host(),
                ses.service(),
                ses.transfered_bytes_to_local(),
                ses.transfered_bytes_to_remote(),
                sessions_.size()));
        sessions_.erase(it);
    }
}

void socks5_stream_manager::on_close(stream_ptr stream)
{
    stop(stream);
}

void socks5_stream_manager::on_error(net::error_code ec, server_stream_ptr stream)
{
    if (auto it = sessions_.find(stream->id()); it != sessions_.end())
        it->second.session.handle_server_error(ec);
}

void socks5_stream_manager::on_error(net::error_code ec, client_stream_ptr stream)
{
    if (auto it = sessions_.find(stream->id()); it != sessions_.end())
        it->second.session.handle_client_error(ec);
}

void socks5_stream_manager::on_accept(server_stream_ptr upstream)
{
    upstream->start();
    const auto id{upstream->id()};
    logger_.debug(std::format("[{}] session created", id));

    auto downstream = std::make_shared<tcp_client_stream>(shared_from_this(), id, upstream->context(), logger_factory_);

    socks5_session session{id, shared_from_this(), logger_factory_};
    socks_pair pair{id, std::move(upstream), std::move(downstream), std::move(session)};
    sessions_.insert({id, std::move(pair)});
}

void socks5_stream_manager::on_read(io_buffer buffer, server_stream_ptr stream)
{
    if (auto it = sessions_.find(stream->id()); it != sessions_.end())
        it->second.session.handle_server_read(buffer);
}

void socks5_stream_manager::on_write(io_buffer buffer, server_stream_ptr stream)
{
    if (auto it = sessions_.find(stream->id()); it != sessions_.end())
        it->second.session.handle_server_write(buffer);
}

void socks5_stream_manager::read_server(int id)
{
    if (auto it = sessions_.find(id); it != sessions_.end())
        it->second.server->read();
}

void socks5_stream_manager::write_server(int id, io_buffer buffer)
{
    if (auto it = sessions_.find(id); it != sessions_.end())
        it->second.server->write(std::move(buffer));
}

void socks5_stream_manager::on_read(io_buffer buffer, client_stream_ptr stream)
{
    if (auto it = sessions_.find(stream->id()); it != sessions_.end())
        it->second.session.handle_client_read(buffer);
}

void socks5_stream_manager::on_write(io_buffer buffer, client_stream_ptr stream)
{
    if (auto it = sessions_.find(stream->id()); it != sessions_.end())
        it->second.session.handle_client_write(buffer);
}

void socks5_stream_manager::on_connect(io_buffer buffer, client_stream_ptr stream)
{
    if (auto it = sessions_.find(stream->id()); it != sessions_.end())
        it->second.session.handle_client_connect(buffer);
}

void socks5_stream_manager::read_client(int id)
{
    if (auto it = sessions_.find(id); it != sessions_.end())
        it->second.client->read();
}

void socks5_stream_manager::write_client(int id, io_buffer buffer)
{
    if (auto it = sessions_.find(id); it != sessions_.end())
        it->second.client->write(std::move(buffer));
}

void socks5_stream_manager::connect(int id, std::string host, std::string service)
{
    if (auto it = sessions_.find(id); it != sessions_.end()) {
        it->second.client->set_host(std::move(host));
        it->second.client->set_service(std::move(service));
        it->second.client->start();
    }
}

