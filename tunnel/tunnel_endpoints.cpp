#include "tunnel/tunnel_endpoints.hpp"
#include <boost/make_shared.hpp>
#include <clue.hpp>

using namespace fuproxy;

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
	LOG_DEBUG("ev_tab karşı tarafı bekliyor...");
	
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
		LOG_DEBUG("ev_tab read: Hata: " << err.message());
		if(err == boost::asio::error::eof || err == boost::asio::error::connection_reset)
		{
			stats_write.lock();
			stats_table.erase(conn);
			stats_write.unlock();
		}
		return;
	}

	stats_write.lock();

	auto result_it = stats_table.find(conn);
	if(result_it == stats_table.end()) {
		LOG_CRITICAL("ev_tab read: Verilen pointer için veri bulunamadı!");
	}
	connection_stats *stats = &result_it->second;

	//Hala veri alıyoruz
	if(stats->receiving && stats->received < stats->to_be_received)
	{
		stats->received += len;

		if(stats->received >= stats->to_be_received)
		{
			///TODO: Bu kötü bir yöntem. Daha temiz hale getir
			LOG_DEBUG("ev_tab read: Yeterince okudu v2");
			stats->received = 0;
			stats->to_be_received = 0;
			stats->receiving = false;
			stats_write.unlock();
			buf.consume(sizeof(universal_header));
			target->packet_in(conn, buf);
		}
		else LOG_DEBUG("ev_tab read: Hala okuyor...");

		stats_write.unlock();
		conn->async_read_some();
		//Başka bir sınıf bu bağlantıyı elinde tutmalı yoksa silinir
	}
	//Yeterince veri aldık
	else if(stats->receiving && stats->received >= stats->to_be_received)
	{
		LOG_DEBUG("ev_tab read: Yeterince okudu v1");
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
		LOG_DEBUG("ev_tab read: Yeni veri: " << len);
		if(len < sizeof(universal_header))
		{
			LOG_DEBUG("ev_tab read: Veri çok az: ");
			stats_write.unlock();
			conn->disconnect();
			return;
		}
		
		const universal_header *mask = static_cast<const universal_header*>(buf.data().data());
		
		if(mask->signature != universal_header::SIGN)
		{
			LOG_DEBUG("ev_tab read: Geçersiz imza: " << std::hex << mask->signature);
			conn->disconnect();
			stats_write.unlock();
			return;
		}

		stats->to_be_received = mask->len;
		stats->received = len - sizeof(universal_header);
		stats->receiving = true;

		if(stats->received == stats->to_be_received)
		{
			LOG_DEBUG("ev_tab read: Tüm paket tek seferde geldi");
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
			LOG_DEBUG("ev_tab read: gereğinden fazla veri var, güvenilmez");
			conn->disconnect();
		}
		else conn->async_read_some();

		LOG_DEBUG("ev_tab read: Hala okunacak veri var");
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
	LOG_DEBUG("tunnel_entry dinlemeyi deniyor...");
	server.start_accept();
	LOG_DEBUG("tunnel_entry dinleme başarılı");
}

/*****************************************************************************/
//Tunnel Exit

tunnel_exit::tunnel_exit(
	boost::asio::io_context &io_ctx,
	boost::asio::ssl::context &ssl_ctx
)
	: io_context(io_ctx),
	ssl_context(ssl_ctx)
{

}

tunnel_exit::~tunnel_exit()
{

}

boost::shared_ptr<tls_connection> tunnel_exit::connect_secure_async(
	const std::string &host,
	unsigned short port,
	tls_connection::callback_table_t *const cb_table
)
{
	return boost::shared_ptr<tls_connection>(nullptr);
}

/*void tunnel_exit::write_async(const boost::shared_ptr<tls_connection> &conn)
{

}*/


