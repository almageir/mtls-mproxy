#include "socks_stream_manager.h"
#include "transport/tcp_client_stream.h"
#include "transport/udp_client_stream.h"

namespace mtls_mproxy
{
    SocksStreamManager::SocksStreamManager(asynclog::LoggerFactory log_factory, bool udp_enabled)
        : logger_factory_{log_factory}
        , logger_{logger_factory_.create("socks5_session_manager")}
        , is_udp_associate_mode_enabled_{udp_enabled}
    {
    }

    void SocksStreamManager::stop(stream_ptr stream)
    {
        stop(stream->id());
    }

    void SocksStreamManager::stop(int id)
    {
        if (auto it = sessions_.find(id); it != sessions_.end()) {
            if (it->second.client)
                it->second.client->stop();
            if (it->second.server)
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

    void SocksStreamManager::on_close(stream_ptr stream)
    {
        stop(stream);
    }

    void SocksStreamManager::on_error(net::error_code ec, ServerStreamPtr stream)
    {
        if (auto it = sessions_.find(stream->id()); it != sessions_.end())
            it->second.session.handle_server_error(ec);
    }

    void SocksStreamManager::on_error(net::error_code ec, ClientStreamPtr stream)
    {
        if (auto it = sessions_.find(stream->id()); it != sessions_.end())
            it->second.session.handle_client_error(ec);
    }

    void SocksStreamManager::on_accept(ServerStreamPtr upstream)
    {
        upstream->start();

        const auto id{upstream->id()};
        logger_.debug(std::format("[{}] session created", id));

        SocksSession session{id, shared_from_this(), logger_factory_};
        session.support_udp_associate_mode(is_udp_associate_mode_enabled_);
        SocksPair pair{id, upstream, nullptr, std::move(session)};
        sessions_.insert({id, std::move(pair)});
        //on_server_ready(upstream);
    }

    void SocksStreamManager::on_read(IoBuffer buffer, ServerStreamPtr stream)
    {
        if (auto it = sessions_.find(stream->id()); it != sessions_.end())
            it->second.session.handle_server_read(buffer);
    }

    void SocksStreamManager::on_write(IoBuffer buffer, ServerStreamPtr stream)
    {
        if (auto it = sessions_.find(stream->id()); it != sessions_.end())
            it->second.session.handle_server_write(buffer);
    }

    void SocksStreamManager::read_server(int id)
    {
        if (auto it = sessions_.find(id); it != sessions_.end())
            it->second.server->read();
    }

    void SocksStreamManager::write_server(int id, IoBuffer buffer)
    {
        if (auto it = sessions_.find(id); it != sessions_.end())
            it->second.server->write(std::move(buffer));
    }

    void SocksStreamManager::on_server_ready(ServerStreamPtr stream)
    {
        const auto sid = stream->id();
        if (auto it = sessions_.find(sid); it != sessions_.end())
            it->second.session.handle_on_accept();
    }

    void SocksStreamManager::on_read(IoBuffer buffer, ClientStreamPtr stream)
    {
        if (auto it = sessions_.find(stream->id()); it != sessions_.end())
            it->second.session.handle_client_read(buffer);
    }

    void SocksStreamManager::on_write(IoBuffer buffer, ClientStreamPtr stream)
    {
        if (auto it = sessions_.find(stream->id()); it != sessions_.end())
            it->second.session.handle_client_write(buffer);
    }

    void SocksStreamManager::on_connect(IoBuffer buffer, ClientStreamPtr stream)
    {
        if (auto it = sessions_.find(stream->id()); it != sessions_.end())
            it->second.session.handle_client_connect(buffer);
    }

    void SocksStreamManager::read_client(int id)
    {
        if (auto it = sessions_.find(id); it != sessions_.end())
            it->second.client->read();
    }

    void SocksStreamManager::write_client(int id, IoBuffer buffer)
    {
        if (auto it = sessions_.find(id); it != sessions_.end())
            it->second.client->write(std::move(buffer));
    }

    void SocksStreamManager::connect(int id, std::string host, std::string service)
    {
        if (auto it = sessions_.find(id); it != sessions_.end()) {
            if (!it->second.client) {
                if (it->second.session.is_udp_mode_enabled()) {
                    it->second.client = std::make_shared<UdpClientStream>(shared_from_this(),
                                                                          id,
                                                                          it->second.server->executor(),
                                                                          logger_factory_);
                } else {
                    it->second.client = std::make_shared<TcpClientStream>(shared_from_this(),
                                                                          id,
                                                                          it->second.server->executor(),
                                                                          logger_factory_);
                }
            }
            it->second.client->set_host(std::move(host));
            it->second.client->set_service(std::move(service));
            it->second.client->start();
        }
    }

    std::vector<std::uint8_t> SocksStreamManager::udp_associate(int id)
    {
        if (auto it = sessions_.find(id); it != sessions_.end())
            return it->second.server->udp_associate();

        return {};
    }
}