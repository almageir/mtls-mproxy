#include "tcp_client_stream.h"
#include "stream_manager.h"

#include <asio/write.hpp>
#include <asio/connect.hpp>

namespace
{
    namespace net = asio;
    using tcp = asio::ip::tcp;

    enum : std::int32_t { eRemote, eLocal };
    std::string ep_to_str(const tcp::socket& sock, std::int32_t dir)
    {
        if (!sock.is_open())
            return "socket not opened";

        net::error_code ec;
        const auto& rep = (dir == eRemote) ? sock.remote_endpoint(ec) : sock.local_endpoint(ec);
        if (ec)
        {
            std::stringstream ss;
            ss << ((dir == eRemote) ? "remote_endpoint failed: " : "local_endpoint failed: ");
            ss << ec.message();
            return ss.str();
        }

        return { rep.address().to_string() + ":" + std::to_string(rep.port()) };
    }
}

namespace mtls_mproxy
{
    TcpClientStream::TcpClientStream(const StreamManagerPtr& ptr,
                                     int id, net::any_io_executor ctx,
                                     const asynclog::LoggerFactory& logger_factory)
        : ClientStream{ptr, id}
        , socket_{ctx}
        , resolver_{ctx}
        , logger_{logger_factory.create("tcp_client")}
        , read_buffer_{}, write_buffer_{}
    {
    }

    TcpClientStream::~TcpClientStream()
    {
         logger_.debug(std::format("[{}] tcp client stream closed ({}:{})", id(), host_, port_));
    }

    std::shared_ptr<TcpClientStream> TcpClientStream::create(const StreamManagerPtr &ptr,
                                                             int id,
                                                             net::any_io_executor ctx,
                                                             const asynclog::LoggerFactory &log_factory)
    {
        return std::shared_ptr<TcpClientStream>(new TcpClientStream(ptr, id, std::move(ctx), log_factory));
    }

    void TcpClientStream::do_start()
    {
        resolver_.async_resolve(
            host_, port_,
            [this, self{shared_from_this()}](const net::error_code& ec, tcp::resolver::results_type results) {
            if (!ec) {
                do_connect(std::move(results));
            }
            else {
                handle_error(ec);
            }
        });
    }

    void TcpClientStream::do_stop()
    {
        net::error_code ignored_ec;
        socket_.shutdown(tcp::socket::shutdown_both, ignored_ec);
    }

    void TcpClientStream::do_connect(tcp::resolver::results_type&& results)
    {
        net::async_connect(
            socket_, results,
            [this, self{shared_from_this()}](const net::error_code& ec, const tcp::endpoint& ep) {
            if (!ec) {
                logger_.info(std::format("[{}] connected to [{}] --> [{}]", id(), host_, ep_to_str(socket_, eRemote)));
                logger_.debug(std::format("[{}] local address [{}]", id(), ep_to_str(socket_, eLocal)));
                IoBuffer event{};
                manager()->on_connect(std::move(event), self);
            }
            else {
                handle_error(ec);
            }
        });
    }

    void TcpClientStream::do_write(IoBuffer event)
    {
        if (wip_) {
            logger_.debug(std::format("[{}] write in progress", id()));
            return;
        }
        wip_ = true;
        std::ranges::copy(event, write_buffer_.begin());
        net::async_write(
            socket_, net::buffer(write_buffer_, event.size()),
            [this, self{shared_from_this()}](const net::error_code& ec, std::size_t) {
            if (!ec) {
                wip_ = false;
                manager()->on_write(self);
            }
            else {
                handle_error(ec);
            }
        });
    }

    void TcpClientStream::do_read()
    {
        if (rip_) {
            logger_.debug(std::format("[{}] read in progress", id()));
            return;
        }
        rip_ = true;
        socket_.async_read_some(
            net::buffer(read_buffer_.data(), read_buffer_.size()),
            [this, self{shared_from_this()}](const net::error_code& ec, const std::size_t length) {
            if (!ec && length) {
                rip_ = false;
                IoBuffer event{read_buffer_.data(), read_buffer_.data() + length};
                manager()->on_read(std::move(event), self);
            }
            else {
                handle_error(ec);
            }
        });
    }

    void TcpClientStream::handle_error(const net::error_code& ec)
    {
        manager()->on_error(ec, shared_from_this());
    }

    void TcpClientStream::do_set_host(std::string host) { host_.swap(host); }

    void TcpClientStream::do_set_service(std::string service) { port_.swap(service); }
}