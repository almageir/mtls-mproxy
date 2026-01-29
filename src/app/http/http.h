#ifndef MTLS_MPROXY_HTTP_HPP
#define MTLS_MPROXY_HTTP_HPP

#include <iostream>

namespace http
{
    enum request_method : int
    {
        kNone,
        kGet,
        kPost,
        kDelete,
        kUpdate,
        kHead,
        kConnect
    };

    struct request_headers
    {
        request_method method{kNone};

        std::string_view uri;
        std::string_view version;
        std::string_view host;
        std::string_view connection;

        std::string get_host() const;
        std::string get_service() const;
    };

    request_headers get_headers(std::string_view header);
};


#endif // MTLS_MPROXY_HTTP_HPP
