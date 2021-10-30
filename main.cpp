#include <iostream>
#include <iomanip>
#include <boost/make_shared.hpp>
#include <aixlog.hpp> //to_string değerleri değiştirildi, GitHub verisyonundan farklı!
#include "tls_server.hpp"

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

int main(int argc, char **argv) {
    auto aixlog_custom_sink = std::make_shared<AixLog::SinkCallback>(
        AixLog::Severity::trace, &log_callback
    );
    AixLog::Log::init({aixlog_custom_sink});

    boost::asio::io_context io_context;
    boost::asio::ssl::context ssl_context(ssl::context::tls);
    tls_server server(2700, io_context, ssl_context);
    io_context.run();

    return 0;
}