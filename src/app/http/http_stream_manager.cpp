#include "http_stream_manager.h"
#include "transport/tcp_client_stream.h"

namespace mtls_mproxy
{
    HttpStreamManager::HttpStreamManager(const asynclog::LoggerFactory& log_factory)
        : logger_factory_{log_factory}
        , logger_{logger_factory_.create("http_session_manager")}
    {
    }

    void HttpStreamManager::stop(stream_ptr stream)
    {
        stop(stream->id());
    }

    void HttpStreamManager::stop(int id)
    {
        if (const auto it = sessions_.find(id); it != sessions_.end()) {
            it->second.client->stop();
            it->second.server->stop();

            const auto& ses = it->second.session;

            logger_.info(
                std::format("[{}] session closed: [{}:{}] rx_bytes: {}, tx_bytes: {}, live sessions {}",
                            id,
                            ses.host(),
                            ses.service(),
                            ses.transferred_bytes_to_local(),
                            ses.transferred_bytes_to_remote(),
                            sessions_.size()));
            sessions_.erase(it);
        }
    }

    void HttpStreamManager::on_close(stream_ptr stream)
    {
        stop(stream);
    }

    void HttpStreamManager::on_error(net::error_code ec, ServerStreamPtr stream)
    {
        if (const auto it = sessions_.find(stream->id()); it != sessions_.end())
            it->second.session.handle_server_error(ec);
    }

    void HttpStreamManager::on_error(net::error_code ec, ClientStreamPtr stream)
    {
        if (const auto it = sessions_.find(stream->id()); it != sessions_.end())
            it->second.session.handle_client_error(ec);
    }

    void HttpStreamManager::on_accept(ServerStreamPtr upstream)
    {
        const auto id{upstream->id()};
        logger_.debug(std::format("[{}] session created", id));

        // auto downstream = TcpClientStream::create(shared_from_this(), id, upstream->executor(), logger_factory_);

        HttpSession session{id, shared_from_this(), logger_factory_};
        HttpPair pair{id, upstream, nullptr, std::move(session)};
        sessions_.insert({id, std::move(pair)});
        upstream->start();
        // on_server_ready(upstream);
    }

    void HttpStreamManager::on_read(IoBuffer buffer, ServerStreamPtr stream)
    {
        if (const auto it = sessions_.find(stream->id()); it != sessions_.end())
            it->second.session.handle_server_read(buffer);
    }

    void HttpStreamManager::on_write(ServerStreamPtr stream)
    {
        if (const auto it = sessions_.find(stream->id()); it != sessions_.end())
            it->second.session.handle_server_write();
    }

    void HttpStreamManager::read_server(int id)
    {
        if (const auto it = sessions_.find(id); it != sessions_.end())
            it->second.server->read();
    }

    void HttpStreamManager::write_server(int id, IoBuffer buffer)
    {
        if (const auto it = sessions_.find(id); it != sessions_.end())
            it->second.server->write(std::move(buffer));
    }

    void HttpStreamManager::on_server_ready(ServerStreamPtr stream)
    {
        const auto sid = stream->id();
        if (const auto it = sessions_.find(sid); it != sessions_.end()) {
            it->second.client = TcpClientStream::create(shared_from_this(), stream->id(), stream->executor(), logger_factory_);
            it->second.session.handle_on_accept();
        }
    }

    void HttpStreamManager::on_read(IoBuffer buffer, ClientStreamPtr stream)
    {
        if (const auto it = sessions_.find(stream->id()); it != sessions_.end())
            it->second.session.handle_client_read(buffer);
    }

    void HttpStreamManager::on_write(ClientStreamPtr stream)
    {
        if (const auto it = sessions_.find(stream->id()); it != sessions_.end())
            it->second.session.handle_client_write();
    }

    void HttpStreamManager::on_connect(IoBuffer buffer, ClientStreamPtr stream)
    {
        if (const auto it = sessions_.find(stream->id()); it != sessions_.end())
            it->second.session.handle_client_connect(buffer);
    }

    void HttpStreamManager::read_client(int id)
    {
        if (const auto it = sessions_.find(id); it != sessions_.end())
            it->second.client->read();
    }

    void HttpStreamManager::write_client(int id, IoBuffer buffer)
    {
        if (const auto it = sessions_.find(id); it != sessions_.end())
            it->second.client->write(std::move(buffer));
    }

    void HttpStreamManager::connect(int id, std::string host, std::string service)
    {
        if (const auto it = sessions_.find(id); it != sessions_.end()) {
            it->second.client->set_host(std::move(host));
            it->second.client->set_service(std::move(service));
            it->second.client->start();
        }
    }

    std::vector<std::uint8_t> HttpStreamManager::udp_associate(int id)
    {
        return {};
    }
}