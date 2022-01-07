#include "tunnel/router.hpp"
#include <boost/json.hpp>
//#include <boost/json/src.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <iostream>
#include <limits>
#include <clue.hpp>
#include <cppcodec/base64_rfc4648.hpp>
#include "util/util.hpp"
#include "tunnel/uuid_token.hpp"
#include "tunnel/errors.hpp"

using namespace fuproxy;
namespace pt = boost::property_tree;
namespace json = boost::json;

std::string generate_response_json(const std::function<std::string(const std::string&)>& func);
std::string generate_error_json(const std::string &cmd, const json::object &response);
std::string generate_success_json(const std::string &cmd, const json::object &response);

router::router(tunnel_exit *const exit_ptr, authenticator *const auth_ptr)
	: exit(exit_ptr), auth(auth_ptr)
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
	try
	{
		pt::read_json(data_stream, root);
	}
	catch(const std::exception& e)
	{
		LOG_INFO("router in: JSON okuması başarısız: \"" << e.what()
			<< "\", bağlantı ("
			<< endpoint_to_string(src->socket()) << ") kapatılıyor");
		
		src->disconnect();
		return;
	}
	
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
		LOG_INFO("router in: İstemci " << endpoint_to_string(src->socket()) << " geçersiz komut gönderdi");
		json::object val;
		val["message"] = "Geçersiz komut";
		std::string msg = generate_error_json(cmd, val);

		src->async_write_error(msg);
		return;
	}
}

void router::packet_out(/*???*/)
{

}

void router::handle_start_tunnel(connection_events::source_t src, boost::property_tree::ptree payload)
{
	LOG_NOTICE("router start_tunnel: İstemci " << endpoint_to_string(src->socket())
		<< " tünel başlatmak istiyor");
	//command_args nesnesi mevcut mu?
	auto args_opt = payload.get_child_optional("command_args");

	if(!args_opt.has_value()) {
		LOG_WARNING("router start_tunnel: İstemci "
			<< endpoint_to_string(src->socket())
			<< " \"command_args\" anahatarını vermedi");

		json::object obj;
		std::error_code err = errors::tunnel_errors::missing_argument;
		obj["error_code"] = err.value();
		obj["message"] = err.message();
		std::string msg = generate_error_json("start_tunnel", obj);

		src->async_write_error(msg);
		return;
	}

	//user_token mevcut mu?
	pt::ptree args = args_opt.value();
	auto token_opt = args.get_optional<std::string>("user_token");

	if(!token_opt.has_value())
	{
		LOG_WARNING("router start_tunnel: İstemci "
			<< endpoint_to_string(src->socket())
			<< " \"user_token\" anahtarını vermedi");

		json::object obj;
		obj["error_code"] = 2;
		obj["message"] = "user_token mevcut değil";
		std::string msg = generate_error_json("start_tunnel", obj);

		src->async_write_error(msg);
		return;
	}

	//user_token'i oluştur
	uuid_token token;

	//user_token geçerli mi ve bu sunucu için uygun mu?
	auth->async_query_user_token(
		&token,
		[&](basic_token *t, const std::error_code &ec)
		{
			if(ec)
			{
				json::object obj;
				obj["error_code"] = ec.value();
				obj["message"] = ec.message();
				std::string msg = generate_error_json("start_tunnel", obj);
				
				src->async_write_error(msg);
				return;
			}

			json::object obj;
			obj["error_code"] = 0;
			obj["message"] = "Başarılı";
			std::string msg = generate_success_json("start_tunnel", obj);

			src->async_write(msg);
			src->async_read_some();
		}
	);
}

void router::handle_connect(connection_events::source_t src, boost::property_tree::ptree payload)
{
	LOG_INFO("router connect: İstemci "
		<< endpoint_to_string(src->socket()) << " dışarıya bağlanmak istiyor");
	
	auto args_opt = payload.get_child_optional("command_args");

	if(!args_opt.has_value())
	{
		LOG_WARNING("router connect: İstemci "
			<< endpoint_to_string(src->socket())
			<< " \"command_args\" anahtarını vermedi");
		
		json::object obj;
		std::error_code err = errors::tunnel_errors::missing_argument;
		obj["error_code"] = err.value();
		obj["message"] = err.message();
		std::string msg = generate_error_json("connect", obj);

		src->async_write_error(msg);
		return;
	}

	pt::ptree args = args_opt.value();
	auto target_host_opt = args.get_optional<std::string>("to");
	auto target_port_opt = args.get_optional<unsigned short>("port");

	//Kullanıcı gerekli parametreleri vermiş mi?
	if(!target_host_opt.has_value() || !target_port_opt.has_value())
	{
		LOG_WARNING("router connect: İstemci "
			<< endpoint_to_string(src->socket())
			<< " command_args için \"to\" anahtarını vermedi");
		
		json::object obj;
		std::error_code err = errors::tunnel_errors::missing_argument;
		obj["error_code"] = err.value();
		obj["message"] = err.message();
		std::string msg = generate_error_json("connect", obj);

		src->async_write_error(msg);
		return;
	}

	exit->async_connect_unsecure(
		src,
		target_host_opt.value(),
		target_port_opt.value(),
		[=](tunnel_route_information::connection_result_t result){
			if(result.first)
			{
				LOG_NOTICE("router connect: İstemci "
					<< endpoint_to_string(src->socket())
					<< " nin bağlanmak istediği "
					<< target_host_opt.value()
					<< " adrese erişlemedi: " << result.first.message());
				
				json::object obj;
				obj["error_code"] = result.first.value();
				obj["message"] = result.first.message();
				std::string msg = generate_error_json("connect", obj);

				src->async_write_error(msg);
				return;
			}

			LOG_NOTICE("router connect: İstemci "
				<< endpoint_to_string(src->socket())
				<< " nin bağlanmak istediği "
				<< target_host_opt.value()
				<< " adrese bağlanıldı");
			
			json::object obj;
			obj["error_code"] = 0;
			obj["message"] = "başarılı";
			obj["connection_token"] = result.second;

			std::string msg = generate_success_json("connect", obj);
			src->async_write(msg);
			src->async_read_some();
		}
	);
}

void router::handle_connect_response()
{
	
}

// Kod spagetti olduğu için router tek yönlü çalışacak
// İstemciden gelen paketler okunup direkt hedefe gönderilecek
// Hedefden gelen yanıtlar tunnel_exit sınıfında okunup direkt istemciye iletilecek
// tunnel_exit sınıfı client soketine erişimi var
void router::handle_data(connection_events::source_t src, boost::property_tree::ptree payload)
{
	LOG_DEBUG("router data: İstemci "
		<< endpoint_to_string(src->socket()) << " yeni veri gönderdi");

	auto args_opt = payload.get_child_optional("command_args");

	if(!args_opt.has_value())
	{
		LOG_WARNING("router data: İstemci "
			<< endpoint_to_string(src->socket())
			<< " \"command_args\" anahtarını vermedi");
		
		json::object obj;
		std::error_code err = errors::tunnel_errors::missing_argument;
		obj["error_code"] = err.value();
		obj["message"] = err.message();
		std::string msg = generate_error_json("data", obj);

		src->async_write_error(msg);
		return;
	}

	pt::ptree args = args_opt.value();
	auto con_token_opt = args.get_optional<std::string>("connection_token");
	if(!con_token_opt.has_value())
	{
		LOG_WARNING("router data: İstemci "
			<< endpoint_to_string(src->socket())
			<< " \"connection_token\" anahtarını vermedi");
		
		json::object obj;
		std::error_code err = errors::tunnel_errors::missing_argument;
		obj["error_code"] = err.value();
		obj["message"] = err.message();
		std::string msg = generate_error_json("data", obj);

		src->async_write_error(msg);
		return;
	}

	auto data_opt = args.get_optional<std::string>("data");
	if(!data_opt.has_value())
	{
		LOG_WARNING("router data: İstemci "
			<< endpoint_to_string(src->socket())
			<< " \"data\" anahtarını vermedi");
		
		json::object obj;
		std::error_code err = errors::tunnel_errors::missing_argument;
		obj["error_code"] = err.value();
		obj["message"] = err.message();
		std::string msg = generate_error_json("data", obj);

		src->async_write_error(msg);
		return;
	}

	auto route_opt = exit->find_route(con_token_opt.value());
	if(!route_opt.has_value())
	{
		LOG_ALERT("router data: İstemci "
			<< endpoint_to_string(src->socket())
			<< " nin verdiği token listede mevcut değil. Hatalı client olabilir. Bağlantı kapatılıyor...");
		
		json::object obj;
		std::error_code err = errors::tunnel_errors::no_such_token;
		obj["error_code"] = err.value();
		obj["message"] = err.message();
		std::string msg = generate_error_json("data", obj);

		src->async_write_error(msg);
		return;
	}

	tunnel_route_information route = route_opt.value();

	//Paketin nereye gideceğini biliyoruz, data alanını decode edip yolla
	std::string data = data_opt.value();
	std::vector<uint8_t> data_decoded = cppcodec::base64_rfc4648::decode(data);

	route.target->async_write_unsecure(data_decoded);
}