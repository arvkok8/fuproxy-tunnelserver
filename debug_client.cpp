#include <iostream>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/array.hpp>

namespace ip = boost::asio::ip;
namespace ssl = boost::asio::ssl;

int main(int argc, char **argv)
{
	boost::array<char, 2048> buf;
	boost::asio::io_context io_context;
	ssl::context ssl_context(ssl::context::tls_client);

	ssl_context.load_verify_file("fuproxy.crt");
	ssl_context.set_verify_mode(ssl::verify_peer);

	ssl::stream<ip::tcp::socket> stream(io_context, ssl_context);

	ip::tcp::resolver resolver(io_context);
	auto endpoints = resolver.resolve(argv[1], argv[2]);

	boost::asio::connect(stream.next_layer(), endpoints);

	stream.handshake(ssl::stream_base::client);

	while(1)
	{
		boost::asio::read(stream, boost::asio::buffer(buf));

		std::cout << "data: " << buf.data() << std::endl;
		buf.fill(0);
	}
	
	return 0;
}