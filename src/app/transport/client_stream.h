#ifndef CLIENT_STREAM_H
#define CLIENT_STREAM_H

#include "stream.h"

#include <string>

class client_stream
    : public stream
    , public std::enable_shared_from_this<client_stream>
{
public:
    client_stream(const stream_manager_ptr& smp, int id) : stream(smp, id) {}

    void set_host(std::string host) { do_set_host(std::move(host)); }
    void set_service(std::string service) { do_set_service(std::move(service)); }

private:
    virtual void do_set_host(std::string host) = 0;
    virtual void do_set_service(std::string service) = 0;
};

using client_stream_ptr = std::shared_ptr<client_stream>;



#endif //CLIENT_STREAM_H
