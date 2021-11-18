#pragma once
#include <boost/asio.hpp>
#include "util/tls_server.hpp"
#include <boost/shared_ptr.hpp>

struct tunnel_route_information
{
public:
    boost::asio::ip::tcp::endpoint from, to;
    boost::shared_ptr<tls_connection> connection;

    bool operator==(const tunnel_route_information &r)
    {
        if(r.from == from && r.to == to && connection == r.connection) return true;
        else return false;
    }
};