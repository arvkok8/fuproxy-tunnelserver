#include "tunnel/router.hpp"
#include <boost/json.hpp>

using namespace fuproxy;

router::router()
{

}

router::~router()
{

}

void router::packet_in(connection_events<void>::buffer_t &buf)
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
}

void router::packet_out(/*???*/)
{

}