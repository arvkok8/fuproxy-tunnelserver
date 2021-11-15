#include <iostream>
#include <iomanip>
#include <boost/make_shared.hpp>
#include <clue.hpp>
#include "tunnel/tunnel_endpoints.hpp"
#include "tunnel/router.hpp"
#include "tunnel/authenticator.hpp"

namespace ssl = boost::asio::ssl;
using namespace fuproxy;

int main(int argc, char **argv) {
    if(argc != 2)
    {
        std::cerr << "Kullanım: tunnel_server <port>" << std::endl;
        return 1;
    }
    
    boost::asio::io_context io_context;
    boost::asio::ssl::context ssl_context(ssl::context::tls_server);

    ssl_context.use_certificate_chain_file("fuproxy.crt");
    ssl_context.use_rsa_private_key_file("fuproxy_private.key", ssl::context::file_format::pem);
    ssl_context.load_verify_file("fuproxy.crt");
    ssl_context.set_verify_mode(ssl::context::verify_fail_if_no_peer_cert);

    /*tls_server server(
        std::stoi(argv[1]),
        io_context,
        ssl_context,
        cb_table
    );*/
    

    tunnel_exit exit(
        io_context,
        ssl_context
    );
    authenticator auth;
    router _router(&exit, &auth);
    tunnel_entry entry(std::stoi(
        argv[1]),
        io_context,
        ssl_context,
        (tunnel_entry::entry_target*)&_router
    );

    entry.start_listen();
    io_context.run();

    return 0;
}