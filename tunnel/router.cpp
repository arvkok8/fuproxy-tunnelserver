#include "tunnel/router.hpp"
#include <boost/json.hpp>
#include <iostream>
#include <limits>

using namespace fuproxy;

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

	std::string data;
	data.assign((const char*)buf.data().data(), buf.size());

	
	
	buf.consume(std::numeric_limits<size_t>::max());
	src->disconnect();
}

void router::packet_out(/*???*/)
{

}