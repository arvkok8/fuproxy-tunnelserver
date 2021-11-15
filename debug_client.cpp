#include <iostream>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/array.hpp>
#include <boost/json.hpp>
#include <boost/json/src.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace ip = boost::asio::ip;
namespace ssl = boost::asio::ssl;

struct universal_header
{
	static const uint32_t SIGN = 0x80542013; //Kesinlikle okul numarası değil
	uint32_t signature;
	uint16_t len;
} __attribute__((packed));

namespace pt = boost::property_tree;

int main(int argc, char **argv)
{
	/*const char* jsondata = "{\"command\": \"hang\", \"command_args\":{\"ab123\": true}}";
	boost::json::value parsedjson = boost::json::parse(jsondata);
	std::stringstream ss;
	ss << jsondata;

	pt::ptree root;
	pt::read_json(ss, root);

	for(pt::ptree::value_type &v : root.get_child(""))
	{
		std::cout << v.first.data() << std::endl;
	}

	std::cout << root.get_child("command_args").get<bool>("ab123") << std::endl;

	return 1;*/

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

	std::string tunnel_request = R"({"command":"start_tunnel","command_args":{"user_token":"param olsa da ben alsam"}})";

	std::vector<unsigned char> datafin(tunnel_request.size() + 6);
	std::copy(tunnel_request.begin(), tunnel_request.end(), datafin.begin() + 6);

	//std::cout << (const char*)&datafin[0];

	universal_header *mask = (universal_header*)&datafin[0];
	mask->signature = mask->SIGN;
	mask->len = datafin.size() - 6;

	//boost::asio::write(stream, boost::asio::buffer(data1));
	boost::asio::write(stream, boost::asio::buffer(datafin));
	boost::asio::read(stream, boost::asio::buffer(buf), boost::asio::transfer_at_least(1));

	std::cout << "data: " << buf.data() << std::endl;

	/*while(1)
	{
		boost::asio::write(stream, boost::asio::buffer(data1));
		boost::asio::write(stream, boost::asio::buffer(data2));
		boost::asio::read(stream, boost::asio::buffer(buf));

		std::cout << "data: " << buf.data() << std::endl;
		buf.fill(0);
	}*/
	
	return 0;
}