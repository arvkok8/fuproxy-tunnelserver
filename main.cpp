#include <iostream>
#include <iomanip>
#include <boost/make_shared.hpp>
#include <aixlog.hpp> //to_string değerleri değiştirildi, GitHub verisyonundan farklı!
#include "util/tls_server.hpp"

namespace ssl = boost::asio::ssl;

static int logging_level = 0;
static bool log_source = true;
static void log_callback(const AixLog::Metadata &metadata, const std::string &msg)
{
    std::string sev_str = AixLog::to_string(metadata.severity);

    std::cout << metadata.timestamp.to_string()
        << std::string(std::max(7 - sev_str.length(), 0ul), ' ')
        << " " << sev_str << ":";
    
    if(log_source)
    {
        std::cout << " <from " << metadata.function.name << " @ "
            << metadata.function.file << ":" << metadata.function.line << ">";
    }
    
    std::cout << " " << msg << std::endl;
}

class debug_cb_table : public tls_connection_events<tls_connection::pointer_t>
{
public:
    void connect(const tls_connection::pointer_t &){}

    void handshake(const tls_connection::pointer_t &conn){
        conn->write_async("param olsa da ben alsam");
    }

    void read(
        const tls_connection::pointer_t &,
        boost::array<char, 1024> &b,
        const boost::system::error_code &e,
        size_t len
    ){}

    void write_done(
        const tls_connection::pointer_t &conn,
        const boost::system::error_code &e,
        size_t len
    ){
        this->handshake(conn);
    }
};

int main(int argc, char **argv) {
    if(argc != 2)
    {
        std::cerr << "Kullanım: tunnel_server <port>" << std::endl;
        return 1;
    }

    auto aixlog_custom_sink = std::make_shared<AixLog::SinkCallback>(
        AixLog::Severity::trace, &log_callback
    );
    AixLog::Log::init({aixlog_custom_sink});

    /******************************************************************************
     * https://youtu.be/CGwibPdEOVk?t=236
     * TODO: Bu yöntem typedeflerle bile çok kirli ve okumayı zorlaştırıyor
     * Çözüm olarak ya tls_connection_events_i classı tls_server.hpp içine atılacak
     * ya da bu kadar kötü template parametreleri gerektirmeyen bir çözüm bulunacak
    */
    boost::shared_ptr<tls_connection_events<tls_connection::pointer_t>> cb_table
        = boost::make_shared<debug_cb_table>(debug_cb_table());
    /*****************************************************************************/
    
    boost::asio::io_context io_context;
    boost::asio::ssl::context ssl_context(ssl::context::tls_server);

    ssl_context.use_certificate_chain_file("fuproxy.crt");
    ssl_context.use_rsa_private_key_file("fuproxy_private.key", ssl::context::file_format::pem);
    ssl_context.load_verify_file("fuproxy.crt");
    ssl_context.set_verify_mode(ssl::context::verify_fail_if_no_peer_cert);

    tls_server server(
        std::stoi(argv[1]),
        io_context,
        ssl_context,
        cb_table
    );

    io_context.run();

    return 0;
}