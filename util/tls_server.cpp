#include "tls_server.hpp"
#include <boost/bind/bind.hpp>
#include <clue.hpp>
#include "util/util.hpp"

//TLS Connection

namespace ip = boost::asio::ip;
namespace ssl = boost::asio::ssl;

tls_connection::pointer_t tls_connection::create(
	boost::asio::io_context &io_context,
	ssl::context &ssl_context,
	tls_connection::callback_table_t *const cb_table
)
{
	return pointer_t(new tls_connection(io_context, ssl_context, cb_table));
}

tls_connection::tls_connection(
	boost::asio::io_context &io_context,
	ssl::context &ssl_context,
	callback_table_t *const cb_table
)
	: secure_stream(io_context, ssl_context),
	callback_table(cb_table),
	read_buffer_view(read_buffer)
{
	
}

void tls_connection::start()
{
	if (callback_table) callback_table->connect(shared_from_this()); 

	LOG_DEBUG("tls_connection[" << (void*)this << "] SSL Handshake bekliyor...");

	secure_stream.async_handshake(
		ssl::stream_base::server,
		boost::bind(
			&tls_connection::handle_handshake,
			shared_from_this(),
			boost::asio::placeholders::error
		)
	);
}

void tls_connection::handle_handshake(const boost::system::error_code &err)
{
	if (err)
	{
		LOG_DEBUG("tls_connection[" << (void*)this << "] IP "
			<< endpoint_to_string(secure_stream.lowest_layer())
			<< " İstemci ile SSL Handshake başarısız, bağlantı kapatılıyor: " << err.message());
		return;
	}

	LOG_DEBUG("tls_connection[" << (void*)this << "] IP "
		<< endpoint_to_string(secure_stream.lowest_layer())
		<< " İstemci ile SSL Handshake başarılı");

	if (callback_table) callback_table->handshake(shared_from_this());
}

void tls_connection::async_read_some()
{
	boost::asio::async_read(
		secure_stream,
		read_buffer_view,
		boost::asio::transfer_at_least(1),
		boost::bind(
			&tls_connection::handle_read, shared_from_this(),
			boost::asio::placeholders::error,
			boost::asio::placeholders::bytes_transferred
		)
	);
}

void tls_connection::disconnect()
{
	secure_stream.lowest_layer().close();
	//secure_stream.shutdown();
	//secure_stream.lowest_layer().shutdown(ip::tcp::socket::shutdown_both);
}

void tls_connection::handle_read(const boost::system::error_code &err, size_t len)
{
	if (callback_table) callback_table->read(shared_from_this(), read_buffer_view, err, len);
}

void tls_connection::handle_write(const boost::system::error_code &err, size_t len)
{
	if (callback_table) callback_table->write_done(shared_from_this(), err, len);
}

tls_connection::stream_t& tls_connection::stream()
{
	return secure_stream;
}

tls_connection::stream_t::next_layer_type& tls_connection::socket()
{
	return secure_stream.next_layer();
}

//TLS Server

tls_server::tls_server(
	unsigned short listen_port,
	boost::asio::io_context &io_ctx,
	ssl::context &ssl_ctx,
	tls_connection::callback_table_t *const cb_table
)
	: io_context(io_ctx),
	ssl_context(ssl_ctx),
	acceptor(io_context, ip::tcp::endpoint(ip::tcp::v4(), listen_port)),
	callback_table(cb_table)
{
	start_accept();
}

void tls_server::start_accept()
{
	tls_connection::pointer_t new_connection = tls_connection::create(io_context, ssl_context, callback_table);

	LOG_DEBUG("tls_server dinlemeye başladı...");

	acceptor.async_accept(new_connection->socket(), boost::bind(
		&tls_server::handle_accept, this, new_connection,
		boost::asio::placeholders::error
	));
}

void tls_server::handle_accept(
	const tls_connection::pointer_t &new_connection,
	const boost::system::error_code &err
)
{	
	if (err)
	{
		LOG_DEBUG("tls_server IP " << endpoint_to_string(new_connection->stream().lowest_layer())
			<< "ile async_accept başarısız: " << err.message());
		return;
	}

	LOG_DEBUG("Yeni istemci: "
		<< endpoint_to_string(new_connection->stream().lowest_layer())
		<< " SSL Handshake bekleniyor...");
	
	new_connection->start();
	start_accept();
}
