#include "http.h"

#include <vector>

namespace
{
    const std::string kHost = "Host:";
    const std::string kConnection = "Connection:";

    std::vector<std::string_view> split(std::string_view str, std::string_view delimeter)
    {
        std::string_view::size_type cur_pos{};
        std::string_view::size_type next_pos{};

        std::vector<std::string_view> items;

        while ((next_pos = str.find_first_of(delimeter, cur_pos)) != std::string_view::npos) {
            if ((next_pos - cur_pos) > 0)
                items.emplace_back(std::string_view{str.data() + cur_pos, next_pos - cur_pos});
            cur_pos = next_pos + 1;
        }

        if (cur_pos < str.size())
            items.emplace_back(std::string_view{str.data() + cur_pos, str.size() - cur_pos});

        return items;
    }

    std::string_view rtrim_copy(std::string_view str, std::string_view pattern)
    {
        if (const auto pos = str.rfind(pattern.data()); pos != std::string_view::npos)
            return std::string_view{str.data(), pos};

        return str;
    }

    bool get_request_fields(std::string_view header, http::request_headers& req)
    {
        auto fields = split(header, " ");
        if (fields.size() != 3)
            return false;

        if (fields[0].empty() || fields[1].empty() || fields[2].empty())
            return false;

        if (fields[0] == "GET")
            req.method = http::request_method::kGet;
        else if (fields[0] == "POST")
            req.method = http::request_method::kPost;
        else if (fields[0] == "DELETE")
            req.method = http::request_method::kDelete;
        else if (fields[0] == "UPDATE")
            req.method = http::request_method::kUpdate;
        else if (fields[0] == "HEAD")
            req.method = http::request_method::kHead;
        else if (fields[0] == "CONNECT") {
            req.method = http::request_method::kConnect;
            req.host = fields[1];
        } else {
            req.method = http::request_method::kNone;
            return std::string_view::npos;
        }

        req.uri = fields[1];
        req.version = fields[2];

        return true;
    }

    std::string_view get_http_header_value(std::string_view name, std::string_view str)
    {
        if (const auto pos = str.find(name); pos != std::string_view::npos)
            return std::string_view{str.data() + name.size() + pos + 1, str.size() - name.size() - pos - 1};

        return {};
    }

    bool get_remaining_required_fields(std::vector<std::string_view> http_headers, std::size_t start_idx, http::request_headers& req)
    {
        for (auto idx = start_idx; idx < http_headers.size(); ++idx) {
            if (req.host.empty()) {
                if (const auto pos = http_headers[idx].find(kHost); pos != std::string_view::npos)
                    req.host = get_http_header_value(kHost, http_headers[idx]);
            } else if (const auto pos = http_headers[idx].find(kConnection); pos != std::string_view::npos) {
                req.connection = get_http_header_value(kConnection, http_headers[idx]);
            }
        }

        return !req.host.empty();
    }
}

namespace http
{
    std::string request_headers::get_service() const
    {
        if (method != request_method::kConnect) {
            const auto parts = split(uri, ":");
            if (parts.size() == 2)
                return std::string{parts[0]};
            return {};
        }

        const auto parts = split(host, ":");
        if (parts.size() == 2)
            return std::string{parts[1]};

        return {};
    }

    std::string request_headers::get_host() const
    {
        if (method != request_method::kConnect)
            return std::string{host};

        const auto parts = split(host, ":");
        if (parts.size() == 2)
            return std::string{parts[0]};

        return {};
    }

    request_headers get_headers(std::string_view header)
    {
        const auto http_req_hdrs = split(header, "\r\n");
        if (http_req_hdrs.size() < 2)
            return {};

        request_headers req;

        if (!get_request_fields(http_req_hdrs[0], req))
            return {};

        if (!get_remaining_required_fields(http_req_hdrs, 1, req))
            return {};

        return req;
    }
}
