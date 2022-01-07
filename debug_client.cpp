#include <iostream>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/array.hpp>
#include <boost/json.hpp>
#include <boost/json/src.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <cppcodec/base64_rfc4648.hpp>

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
	std::string connect_request = R"({
		"command": "connect",
		"command_args": {
			"to": "violence.de",
			"port": 80
		}
	})";
	std::string data_request_begin = R"({
		"command": "data",
		"command_args": {
			"connection_token": ")";
	std::string data_request_end = R"(",
			"data": "R0VUIC8gSFRUUC8xLjENCkhvc3Q6IHZpb2xlbmNlLmRlDQoNCgo="
		}
	}
	)";
	//^^^^^
	//GET / HTTP/1.1\r\nHost: violence.de\r\n\r\n

	std::vector<unsigned char> datafin(tunnel_request.size() + 6);
	std::copy(tunnel_request.begin(), tunnel_request.end(), datafin.begin() + 6);

	//std::cout << (const char*)&datafin[0];

	universal_header *mask = (universal_header*)&datafin[0];
	mask->signature = mask->SIGN;
	mask->len = datafin.size() - 6;

	std::cout << "sending start_tunnel\n";
	//boost::asio::write(stream, boost::asio::buffer(data1));
	boost::asio::write(stream, boost::asio::buffer(datafin));
	boost::asio::read(stream, boost::asio::buffer(buf), boost::asio::transfer_at_least(1));

	std::cout << "data: " << buf.data() << std::endl;

	datafin = std::vector<uint8_t>(connect_request.size() + 6);
	std::copy(connect_request.begin(), connect_request.end(), datafin.begin() + 6);
	mask = (universal_header*)&datafin[0];
	mask->signature = mask->SIGN;
	mask->len = datafin.size() - 6;

	buf.fill(0);

	sleep(2);

	std::cout << "sending connect\n";
	boost::asio::write(stream, boost::asio::buffer(datafin));
	std::cout << "reading...\n";
	boost::asio::read(stream, boost::asio::buffer(buf), boost::asio::transfer_at_least(1));

	std::cout << "data: " << buf.data() << std::endl;

	//json oku ve oluştur

	std::stringstream ss;
	ss << buf.data();
	pt::ptree root;
	pt::read_json(ss, root);
	auto rootjson = boost::json::parse(ss.str()).as_object();

	auto resp = root.get_child("response");
	std::string token = rootjson["response"].as_object()["connection_token"].as_string().c_str();

	//std::cout << "aeaeae: " << rootjson["response"].as_object()["connection_token"].as_string() << std::endl;

	std::string data_request = data_request_begin;
	data_request.append(token);
	data_request.append(data_request_end);

	datafin = std::vector<uint8_t>(data_request.size() + 6);
	mask = (universal_header*)&datafin[0];
	mask->signature = mask->SIGN;
	mask->len = data_request.length();
	std::copy(data_request.begin(), data_request.end(), datafin.begin() + 6);

	/*std::cout << "ilk " << datafin.size() << " byte:\n";
	for(int i = 0; i < datafin.size(); i++)
	{
		std::cout << std::hex << (unsigned int)datafin[i];
		std::cout << std::endl;
	}*/

	std::cout << "+header: " << data_request << "\n";

	std::cout << "sending data\n";
	
	buf.fill(0);
	boost::asio::write(stream, boost::asio::buffer(datafin));

	std::cout << "listening...\n";

	bool receiving = false;
	int to_be_received = 0;
	int received = 0;
	std::vector<uint8_t> resize_buf;

	while(1)
	{
		size_t len = boost::asio::read(stream, boost::asio::buffer(buf), boost::asio::transfer_at_least(1));
		if(receiving)
		{
			if(to_be_received < received)
			{
				resize_buf.insert(resize_buf.begin(), buf.begin(), buf.begin() + len);
				to_be_received += len;
			}
			else
			{
				std::cout << "done: \n";
			}
		}
		else
		{
			if(len < 6)
			{
				std::cout << "çok kısa\n";
				return 5;
			}

			mask = (universal_header*)buf.data();
			to_be_received = mask->len;
			resize_buf.insert(resize_buf.begin(), buf.begin(), buf.begin() + len);
			received += len;
		}
		
		buf.fill(0);

		if(received >= to_be_received)
		{
			std::stringstream ss;
			resize_buf.push_back(0);

			ss << (const char*)&resize_buf[sizeof(universal_header)];

			//std::cout << "aeeeeeeeeee: " << ss.str() << "\n";
			boost::json::object root = boost::json::parse(ss.str()).as_object();

			std::string datastr = root["command_args"].as_object()["data"].as_string().data();
			std::string safestr;
			auto vec = cppcodec::base64_rfc4648::decode(datastr);

			safestr.insert(safestr.begin(), vec.begin(), vec.end());
			std::cout << safestr << std::endl;
			resize_buf.clear();
		}
	}

	char c;
	std::cin >> c;

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