#include "tunnel/tunnel_endpoints.hpp"
#include <boost/make_shared.hpp>

using namespace fuproxy;

//Tunnel Entry

tunnel_entry::tunnel_entry(
	unsigned short port,
	boost::asio::io_context &io_ctx,
	boost::asio::ssl::context &ssl_ctx,
	entry_target *const ent_target
)
	: ev_table(this, ent_target),
	server(port, io_ctx, ssl_ctx, &ev_table),
	target(ent_target)
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

void tunnel_entry::event_table::connect(const tunnel_entry::event_table::connection_t &conn)
{
	conn->start();
}

void tunnel_entry::event_table::handshake(const tunnel_entry::event_table::connection_t &conn)
{
	conn->read_async();
}

void tunnel_entry::event_table::read(
	const tunnel_entry::event_table::connection_t &,
	connection_events<void>::buffer_t &buf,
	const boost::system::error_code &err,
	size_t len
)
{
	target->packet_in(buf);
}

void tunnel_entry::event_table::write_done(
	const tunnel_entry::event_table::connection_t &,
	const boost::system::error_code &err,
	size_t len
)
{
	target->packet_out();
}

void tunnel_entry::start_listen()
{
	server.start_accept();
}

/*****************************************************************************/
//Tunnel Exit

tunnel_exit::tunnel_exit()
{

}

tunnel_exit::~tunnel_exit()
{

}

boost::shared_ptr<tls_connection>& tunnel_exit::connect_async(
	const std::string &host,
	unsigned short port,
	tls_connection::callback_table_t *const cb_table
)
{

}

/*void tunnel_exit::write_async(const boost::shared_ptr<tls_connection> &conn)
{

}*/


