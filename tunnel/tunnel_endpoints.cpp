#include "tunnel/tunnel_endpoints.hpp"
#include <boost/make_shared.hpp>
#include <boost/json.hpp>
//#include <boost/json/src.hpp>
#include <boost/property_tree/ptree.hpp>
#include <fstream>
#include <random>
#include <ctime>
#include <clue.hpp>
#include <cppcodec/base64_rfc4648.hpp>
#include "util/util.hpp"
#include "tunnel/errors.hpp"

using namespace fuproxy;
namespace ip = boost::asio::ip;
namespace json = boost::json;
using base64 = cppcodec::base64_rfc4648;

//Tunnel Entry

tunnel_entry::tunnel_entry(
	unsigned short port,
	boost::asio::io_context &io_ctx,
	boost::asio::ssl::context &ssl_ctx,
	entry_target *const ent_target
)
	: ev_table(this, ent_target),
	target(ent_target),
	server(port, io_ctx, ssl_ctx, &ev_table)
{

}

tunnel_entry::~tunnel_entry()
{

}

tunnel_entry::event_table::event_table(tunnel_entry *const parent, entry_target *const ent_target)
	: entry_parent(parent),
	target(ent_target)
{

}

void tunnel_entry::event_table::connect(tunnel_entry::event_table::source_t conn)
{

}

void tunnel_entry::event_table::handshake(tunnel_entry::event_table::source_t conn)
{
	LOG_INFO("ev_tab handshake: "
				<< endpoint_to_string(conn->socket())
				<< " İstemcinin komutunu bekliyoruz...");
	
	stats_write.lock();
	connection_stats val = {false, 0, 0};
	stats_table.insert(std::make_pair(conn, val));
	stats_write.unlock();

	conn->async_read_some();
}

//TODO: bunu okuması neredeyse imkansız daha okunabilir bir çözüm bul
void tunnel_entry::event_table::read(
	tunnel_entry::event_table::source_t conn,
	connection_events::buffer_t &buf,
	const boost::system::error_code &err,
	size_t len
)
{
	if(err)
	{
		if(err == boost::asio::error::eof || err == boost::asio::error::connection_reset)
		{
			stats_write.lock();
			stats_table.erase(conn);
			stats_write.unlock();
			LOG_DEBUG("ev_tab read: İstemci "
				<< endpoint_to_string(conn->socket())
				<< " ile bağlantı kapandı");
		}
		else
		{
			LOG_WARNING("ev_tab read: İstemci "
				<< endpoint_to_string(conn->socket())
				<< " Hata: " << err.message());
		}
		return;
	}

	stats_write.lock();

	auto result_it = stats_table.find(conn);
	if(result_it == stats_table.end())
	{
		LOG_ALERT("ev_tab read: "
			<< endpoint_to_string(conn->socket())
			<< " Verilen pointer için veri bulunamadı!");
	}
	connection_stats *stats = &result_it->second;

	//Hala veri alıyoruz
	if(stats->receiving && stats->received < stats->to_be_received)
	{
		stats->received += len;

		if(stats->received >= stats->to_be_received)
		{
			///TODO: Bu kötü bir yöntem. Daha temiz hale getir
			LOG_DEBUG("ev_tab read: "
				<< endpoint_to_string(conn->socket())
				<< " Yeterince okudu");
			stats->received = 0;
			stats->to_be_received = 0;
			stats->receiving = false;
			stats_write.unlock();
			buf.consume(sizeof(universal_header));
			target->packet_in(conn, buf);
		}
		else LOG_DEBUG("ev_tab read: "
			<< endpoint_to_string(conn->socket())
			<< " Hala okuyor...");

		stats_write.unlock();
		conn->async_read_some();
		//Başka bir sınıf bu bağlantıyı elinde tutmalı yoksa silinir
	}
	//Yeterince veri aldık
	else if(stats->receiving && stats->received >= stats->to_be_received)
	{
		LOG_DEBUG("ev_tab read: "
			<< endpoint_to_string(conn->socket())
			<< " Yeterince okudu");
		//Eğer gerektiğinden fazla okuduysak fazla veriyi görmezden gelebiliriz
		stats->received = 0;
		stats->to_be_received = 0;
		stats->receiving = false;
		stats_write.unlock();
		buf.consume(sizeof(universal_header));
		target->packet_in(conn, buf);
	}
	//Yeni veri almaya başladık
	else if(!stats->receiving)
	{
		LOG_DEBUG("ev_tab read: Yeni veri: "
			<< endpoint_to_string(conn->socket())
			<< " Boyut: " << len);
		if(len < sizeof(universal_header))
		{
			LOG_WARNING("ev_tab read: "
				<< endpoint_to_string(conn->socket())
				<< " Veri çok az: ");
			stats_write.unlock();
			conn->disconnect();
			return;
		}
		
		const universal_header *mask = static_cast<const universal_header*>(buf.data().data());
		
		if(mask->signature != universal_header::SIGN)
		{
			LOG_WARNING("ev_tab read: "
				<< endpoint_to_string(conn->socket())
				<< " Geçersiz imza: " << std::hex << mask->signature);
			conn->disconnect();
			stats_write.unlock();
			return;
		}

		stats->to_be_received = mask->len;
		stats->received = len - sizeof(universal_header);
		stats->receiving = true;

		if(stats->received == stats->to_be_received)
		{
			LOG_DEBUG("ev_tab read: "
				<< endpoint_to_string(conn->socket())
				<< " Tüm paket tek seferde geldi");
			stats->received = 0;
			stats->to_be_received = 0;
			stats->receiving = false;
			buf.consume(sizeof(universal_header));
			stats_write.unlock();
			target->packet_in(conn, buf);
			return;
		}
		else if(stats->received > stats->to_be_received)
		{
			LOG_DEBUG("ev_tab read: "
				<< endpoint_to_string(conn->socket())
				<< " Header'da belirtilen veriden fazla veri geldi. Riskli olduğu için kapatılıyor");
			conn->disconnect();
		}
		else conn->async_read_some();

		LOG_DEBUG("ev_tab read: "
			<< endpoint_to_string(conn->socket())
			<< " Hala okunacak veri var");
		stats_write.unlock();
	}
}

void tunnel_entry::event_table::write_done(
	tunnel_entry::event_table::source_t,
	const boost::system::error_code &err,
	size_t len
)
{
	target->packet_out();
}

void tunnel_entry::start_listen()
{
	LOG_DEBUG("tunnel_entry: dinlemeyi deniyor...");
	server.start_accept();
	LOG_DEBUG("tunnel_entry: dinleme başarılı");
}

/*****************************************************************************/
//Tunnel Exit

tunnel_exit::tunnel_exit(
	boost::asio::io_context &io_ctx,
	boost::asio::ssl::context &ssl_ctx
)
	: io_context(io_ctx),
	ssl_context(ssl_ctx),
	token_list(),
	ev_table(*this)
{
	std::srand(std::time(nullptr));
}

tunnel_exit::~tunnel_exit()
{

}

tunnel_exit::event_table::event_table(tunnel_exit &p)
	: exit_parent(p)
{

}

tunnel_exit::event_table::~event_table()
{

}

void tunnel_exit::event_table::connect(tunnel_exit::event_table::source_t conn)
{
	//auto result = exit_parent.connection_list.find("");
	//Koli bandı üstüne daha fazla koli bandı fixler
	tunnel_exit::token_list_t::iterator result = exit_parent.token_list.end();
	for(auto it = exit_parent.token_list.begin(); it != exit_parent.token_list.end(); it++)
	{
		if(it->second == conn)
		{
			result = it;
			break;
		}
	}

	if(result == exit_parent.token_list.end())
	{
		LOG_ALERT("tunnel_exit::event_table connect: tls_connection["
			<< static_cast<void*>(conn.get()) << "] bağlantı listesinde yok! Görmezden geliniyor");
		if(conn != nullptr) conn->disconnect();
		return;
	}

	LOG_DEBUG("tunnel_exit::event_table connect: tls_connection["
		<< (void*)conn.get() << "] "
		<< conn->socket().local_endpoint().address().to_string()
		<< " ile bağlantı kurdu.");
	
	std::error_code ec = static_cast<errors::tunnel_errors>(0);
	auto ret = std::make_pair(ec, result->first);
	auto route = exit_parent.connection_list.find(result->second);

	route->second.cb(ret);
}

void tunnel_exit::event_table::handshake(tunnel_exit::event_table::source_t conn)
{
	
}

void tunnel_exit::event_table::read(
	tunnel_exit::event_table::source_t conn,
	tunnel_exit::event_table::buffer_t &buf,
	const boost::system::error_code &err,
	size_t len
)
{
	if(err)
	{
		if(err == boost::asio::error::eof || err == boost::asio::error::connection_reset)
		{
			LOG_DEBUG("tunnel_exit ev_tab read: İstemci "
				<< endpoint_to_string(conn->socket())
				<< " ile bağlantı kapandı");
		}
		else
		{
			LOG_WARNING("tunnel_exit ev_tab read: İstemci "
				<< endpoint_to_string(conn->socket())
				<< " Hata: " << err.message());
		}

		auto token = exit_parent.connection_list.find(conn);
		if(token != exit_parent.connection_list.end())
		{
			exit_parent.token_list.erase(token->second.token);
			exit_parent.connection_list.erase(conn);
		}
		
		return;
	}

	auto route = exit_parent.connection_list.find(conn);

	if(route == exit_parent.connection_list.end())
	{
		LOG_ALERT("tunnel_exit ev_tab read: rota bilgisi bulunamadı!");
		return;
	}

	//size_t len = buf.data().size();
	std::stringstream ss;
	ss << "{\"command\":\"data\",\"command_args\":{\"connection_token\":\"";
	ss << route->second.token << "\",";
	ss << "\"data\":\"";
	ss << base64::encode(static_cast<const uint8_t*>(buf.data().data()), len);
	ss << "\"}}";

	buf.consume(len);

	std::vector<uint8_t> data_final = std::vector<uint8_t>(ss.str().length() + sizeof(universal_header));
	std::string final_str = ss.str();
	
	data_final.insert(data_final.begin() + sizeof(universal_header), final_str.begin(), final_str.end());
	
	universal_header *mask = (universal_header*)&data_final[0];
	mask->len = ss.str().length();
	mask->signature = mask->SIGN;

	LOG_DEBUG("tunnel_exit ev_tab read: Hedef "
		<< endpoint_to_string(conn->socket())
		<< " sunucudan istemci "
		<< endpoint_to_string(route->second.target->socket())
		<< " ye veri gönderiliyor, boyut: " << len << " + overhead");
	route->second.source->async_write(data_final);

	conn->async_read_some_unsecure();
}

void tunnel_exit::event_table::write_done(
	tunnel_exit::event_table::source_t conn,
	const boost::system::error_code &err,
	size_t len
)
{
	if(err)
	{
		LOG_NOTICE("router data: "
			<< endpoint_to_string(conn->socket())
			<< " paket gönderilemedi: " << err.message()
		);

		json::object obj;
		std::error_code err = errors::tunnel_errors::remote_connection_failed;
		obj["error_code"] = err.value();
		obj["message"] = err.message();
		std::string msg = generate_error_json("data", obj);

		conn->async_write_error("msg");
		return;
	}
	
	LOG_DEBUG("router data: istemci > client");
	conn->async_read_some_unsecure();
	
}

void tunnel_exit::async_connect_secure(
	connection_events::source_t from,
	const std::string &host,
	unsigned short port,
	std::function<void(tunnel_route_information::connection_result_t)> result_cb
)
{
	auto ret = std::make_pair<std::error_code, tunnel_route_information::connection_token_t>(
		static_cast<errors::tunnel_errors>(0), ""
	);
	std::error_code &ret_error = std::get<0>(ret);
	tunnel_route_information::connection_token_t &ret_token = std::get<1>(ret);

	auto conptr = tls_connection::create(io_context, ssl_context, &ev_table);
	
	ip::tcp::resolver resolver(io_context);
	ip::tcp::resolver::results_type endpoints = resolver.resolve(host, std::to_string(port));
	
	if(endpoints.begin() == endpoints.end())
	{
		ret_error = errors::tunnel_errors::name_resolution_failed;
		result_cb(ret);
		return;
	}

	std::string token = generate_connection_token();
	tunnel_route_information route{token, from->socket().remote_endpoint(), *endpoints.begin(), from, conptr, result_cb};

	auto result = token_list.emplace(token, conptr);
	connection_list.emplace(conptr, route);

	if(!std::get<1>(result))
	{
		LOG_CRITICAL("tunnel_exit async_connect_secure: Bağlantı listeye eklenemedi!");
		return;
	}

	conptr->async_connect(endpoints);
}

void tunnel_exit::async_connect_unsecure(
	connection_events::source_t from,
	const std::string &host,
	unsigned short port,
	std::function<void(tunnel_route_information::connection_result_t)> result_cb
)
{
	auto ret = std::make_pair<std::error_code, tunnel_route_information::connection_token_t>(
		static_cast<errors::tunnel_errors>(0), ""
	);
	std::error_code &ret_error = std::get<0>(ret);
	tunnel_route_information::connection_token_t &ret_token = std::get<1>(ret);

	auto conptr = tls_connection::create(io_context, ssl_context, &ev_table);
	
	ip::tcp::resolver resolver(io_context);
	ip::tcp::resolver::results_type endpoints = resolver.resolve(host, std::to_string(port));
	
	if(endpoints.begin() == endpoints.end())
	{
		ret_error = errors::tunnel_errors::name_resolution_failed;
		result_cb(ret);
		return;
	}

	std::string token = generate_connection_token();
	tunnel_route_information route{token, from->socket().remote_endpoint(), *endpoints.begin(), from, conptr, result_cb};

	auto result = token_list.emplace(token, conptr);
	connection_list.emplace(conptr, route);

	if(!std::get<1>(result))
	{
		LOG_CRITICAL("tunnel_exit async_connect_secure: Bağlantı listeye eklenemedi!");
		return;
	}

	conptr->async_connect_unsecure(endpoints);
}

boost::optional<tunnel_route_information> tunnel_exit::find_route(tunnel_route_information::connection_token_t conn)
{
	auto it = token_list.find(conn);

	if(it == token_list.end())
	{
		boost::optional<tunnel_route_information> errval;
		return errval;
	}

	///TODO: Daha sıkı hata kontrolü
	auto route = connection_list.find(it->second);

	return boost::make_optional(route->second);
}

tunnel_exit::connection_token_pod_t tunnel_exit::generate_connection_token_pod()
{
	tunnel_exit::connection_token_pod_t data;
	std::ifstream urand;
	
	urand.open("/dev/urandom", std::ios::binary | std::ios::in);
	
	if(!urand.is_open())
	{
		LOG_ALERT("tunnel_exit generate_connection_token_pod: /dev/urandom açılamadı, std::rand kullanılıyor");
		
		for(int i = 0; i < data.size(); i++)
		{
			data[i] = static_cast<uint8_t>(std::rand() % 255);
		}
		
		return data;
	}

	urand.read((char*)&data[0], data.size());
	//         ^ Nedense static_cast unsigned char* -> char* yapmak istemiyor

	return data;
}

std::string tunnel_exit::generate_connection_token()
{
	tunnel_exit::connection_token_pod_t data = tunnel_exit::generate_connection_token_pod();
	return tunnel_exit::generate_connection_token(data);
}

std::string tunnel_exit::generate_connection_token(const tunnel_exit::connection_token_pod_t &pod)
{
	std::string cpy = base64::encode(pod);
	return cpy;
}
