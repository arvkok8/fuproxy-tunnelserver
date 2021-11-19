#pragma once
#include <boost/asio.hpp>
#include "util/tls_server.hpp"
#include <boost/shared_ptr.hpp>
#include <functional>

struct tunnel_route_information
{
public:
    typedef std::string connection_token_t;
    typedef std::pair<std::error_code, connection_token_t> connection_result_t;
    typedef std::function<void(connection_result_t)> cb_t;

    std::string token;
    boost::asio::ip::tcp::endpoint from, to;
    boost::shared_ptr<tls_connection> connection;
    cb_t cb;

    bool operator==(const tunnel_route_information &r)
    {
        if(r.token == token && r.from == from && r.to == to && r.connection == connection) return true;
        else return false;
    }
};