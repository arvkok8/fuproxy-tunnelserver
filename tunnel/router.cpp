#include "tunnel/router.hpp"
#include <boost/json.hpp>
#include <boost/json/src.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <iostream>
#include <limits>
#include <clue.hpp>
#include "util/util.hpp"

using namespace fuproxy;
namespace pt = boost::property_tree;
namespace json = boost::json;

std::string generate_error_json(const std::function<std::string(std::string)>& func);

router::router(tunnel_exit *const exit_ptr)
	: exit(exit_ptr)
{

}

router::~router()
{

}

void router::packet_in(connection_events::source_t src, connection_events::buffer_t &buf)
{
	/**
	 * JSON verisini oku
	 * komut değerini ayır
	 * 
	 * connect için authenticator'u kullan
	 * 	onay aldıktan sonra tunnel_exit'i kullan
	 * 	bağlantının sonucuna göre paket oluştur ve gönder
	 * 
	 * data için kendi veritabanına bak ve hedefi bul
	 * 	connection token eski değilse paketi hedefine gönder
	*/

	std::stringstream data_stream;
	std::string data, cmd;

	data.assign((const char*)buf.data().data(), buf.size());
	buf.consume(buf.size());
	data_stream << data;

	pt::ptree root;
	pt::read_json(data_stream, root);
	
	try
	{
		cmd = root.get<std::string>("command");
	}
	catch(const std::exception& e)
	{
		LOG_INFO("router in: \"command\" anahtarı eksik. Bağlantı "
			<< endpoint_to_string(src->socket()) << " kapatılıyor");
		src->disconnect();
		return;
	}

	if(cmd == "start_tunnel") handle_start_tunnel(src, root);
	else if(cmd == "connect") handle_connect(src, root);
	else if(cmd == "data") handle_data(src, root);
	else
	{
		std::stringstream error_ss;
		error_ss << "{\"success\": false, \"command\": \""
			<< cmd << "\", \"response\": {"
			<< "\"message\": \"Geçersiz komut\"}"
			<< "}";
		
		src->async_write_error(error_ss.str());
	}
}

void router::packet_out(/*???*/)
{

}

void router::handle_start_tunnel(connection_events::source_t src, boost::property_tree::ptree payload)
{
	
}

void router::handle_connect(connection_events::source_t, boost::property_tree::ptree payload)
{

}

void router::handle_data(connection_events::source_t, boost::property_tree::ptree payload)
{

}

/**
 * @brief İstemciye göndermek için JSON hata mesajı oluştur
 * @param func Paket kökünün her elemanı için çağırılacak fonksyion
 * @return Oluşturulan hata JSON metini
*/
std::string generate_error_json(const std::function<std::string(std::string)>& func)
{
	std::stringstream ss;

	ss << "{";

	ss << "\"success\":" << func("success") << ",";
	ss << "\"command\":" << func("command") << ",";
	ss << "\"command_args\":" << func("command_args") << ",";
	ss << "\"response\":" << func("response");

	ss << "}";

	return ss.str();
}