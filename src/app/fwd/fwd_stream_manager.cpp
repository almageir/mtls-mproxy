#include "fwd_stream_manager.h"
#include "transport/tcp_client_stream.h"

namespace mtls_mproxy
{
    FwdStreamManager::FwdStreamManager(const asynclog::LoggerFactory& log_factory, std::string host, std::string port)
        : logger_factory_{log_factory}
        , logger_{logger_factory_.create("tun_session_manager")}
        , host_{std::move(host)}
        , port_{std::move(port)}
    {
    }

    void FwdStreamManager::stop(stream_ptr stream)
    {
        stop(stream->id());
    }

    void FwdStreamManager::stop(int id)
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

    void FwdStreamManager::on_close(stream_ptr stream)
    {
        stop(stream);
    }

    void FwdStreamManager::on_error(net::error_code ec, ServerStreamPtr stream)
    {
        if (const auto it = sessions_.find(stream->id()); it != sessions_.end())
            it->second.session.handle_server_error(ec);
    }

    void FwdStreamManager::on_error(net::error_code ec, ClientStreamPtr stream)
    {
        if (const auto it = sessions_.find(stream->id()); it != sessions_.end())
            it->second.session.handle_client_error(ec);
    }

    void FwdStreamManager::on_accept(ServerStreamPtr upstream)
    {
        upstream->start();

        const auto id{upstream->id()};
        logger_.debug(std::format("[{}] session created", id));

        FwdSession session{id, shared_from_this(), logger_factory_};
        session.set_endpoint_info(host_, port_);
        FwdPair pair{id, std::move(upstream), nullptr, std::move(session)};
        sessions_.insert({id, std::move(pair)});
    }

    void FwdStreamManager::on_read(IoBuffer buffer, ServerStreamPtr stream)
    {
        if (const auto it = sessions_.find(stream->id()); it != sessions_.end())
            it->second.session.handle_server_read(buffer);
    }

    void FwdStreamManager::on_write(ServerStreamPtr stream)
    {
        if (const auto it = sessions_.find(stream->id()); it != sessions_.end())
            it->second.session.handle_server_write();
    }

    void FwdStreamManager::read_server(int id)
    {
        if (const auto it = sessions_.find(id); it != sessions_.end())
            it->second.server->read();
    }

    void FwdStreamManager::write_server(int id, IoBuffer buffer)
    {
        if (const auto it = sessions_.find(id); it != sessions_.end())
            it->second.server->write(std::move(buffer));
    }

    void FwdStreamManager::on_server_ready(ServerStreamPtr stream)
    {
        const auto sid = stream->id();
        if (const auto it = sessions_.find(sid); it != sessions_.end()) {
            auto downstream = TcpClientStream::create(shared_from_this(),
                                                                sid,
                                                                stream->executor(),
                                                                logger_factory_);
            it->second.client = std::move(downstream);
            it->second.session.handle_on_accept();
        }
    }

    void FwdStreamManager::on_read(IoBuffer buffer, ClientStreamPtr stream)
    {
        if (const auto it = sessions_.find(stream->id()); it != sessions_.end())
            it->second.session.handle_client_read(buffer);
    }

    void FwdStreamManager::on_write(ClientStreamPtr stream)
    {
        if (const auto it = sessions_.find(stream->id()); it != sessions_.end())
            it->second.session.handle_client_write();
    }

    void FwdStreamManager::on_connect(IoBuffer buffer, ClientStreamPtr stream)
    {
        if (const auto it = sessions_.find(stream->id()); it != sessions_.end())
            it->second.session.handle_client_connect(buffer);
    }

    void FwdStreamManager::read_client(int id)
    {
        if (const auto it = sessions_.find(id); it != sessions_.end())
            it->second.client->read();
    }

    void FwdStreamManager::write_client(int id, IoBuffer buffer)
    {
        if (const auto it = sessions_.find(id); it != sessions_.end())
            it->second.client->write(std::move(buffer));
    }

    void FwdStreamManager::connect(int id, std::string host, std::string service)
    {
        if (const auto it = sessions_.find(id); it != sessions_.end()) {
            it->second.client->set_host(std::move(host));
            it->second.client->set_service(std::move(service));
            it->second.client->start();
        }
    }

    std::vector<std::uint8_t> FwdStreamManager::udp_associate(int id)
    {
        return {};
    }
}