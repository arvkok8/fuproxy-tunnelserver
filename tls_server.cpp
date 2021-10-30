#include "tls_server.hpp"
#include <boost/bind/bind.hpp>
#include <aixlog.hpp>
#include "util.hpp"

//TLS Connection

namespace ip = boost::asio::ip;
namespace ssl = boost::asio::ssl;

tls_connection::pointer_t tls_connection::create(
	boost::asio::io_context &io_context,
	ssl::context &ssl_context,
	const tls_connection::callback_table_t &cb_table
)
{
	return pointer_t(new tls_connection(io_context, ssl_context, cb_table));
}

tls_connection::tls_connection(
	boost::asio::io_context &io_context,
	ssl::context &ssl_context,
	const callback_table_t &cb_table
)
	: secure_stream(io_context, ssl_context), callback_table(cb_table)
{
}

void tls_connection::start()
{
	if (callback_table) callback_table->connect(shared_from_this()); 

	secure_stream.async_handshake(
		ssl::stream_base::server,
		boost::bind(&tls_connection::handle_handshake, this, boost::asio::placeholders::error)
	);
}

void tls_connection::handle_handshake(const boost::system::error_code &err)
{
	if (err)
	{
		LOG(DEBUG) << endpoint_to_string(secure_stream.lowest_layer())
			<< " İstemci ile SSL Handshake başarısız, bağlantı kapatılıyor: " << err.message();
		return;
	}

	if (callback_table) callback_table->handshake(shared_from_this());
	LOG(DEBUG) << endpoint_to_string(secure_stream.lowest_layer())
		<< "İstemci ile SSL Handshake başarılı";
}

void tls_connection::start_read()
{
	boost::asio::async_read(secure_stream, boost::asio::buffer(read_buffer),
			boost::bind(
				&tls_connection::handle_read, this,
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred
	));
}

void tls_connection::handle_read(const boost::system::error_code &err, size_t len)
{
	if (callback_table) callback_table->read(shared_from_this(), read_buffer, err, len);
}

void tls_connection::handle_write(const boost::system::error_code &err, size_t len)
{
	if (callback_table) callback_table->write_done(shared_from_this(), err, len);
}

template <typename buffer_t>
void tls_connection::write_async(const buffer_t &buf)
{
	boost::asio::async_write(secure_stream, boost::asio::buffer(buf), boost::bind(
		&tls_connection::handle_write, shared_from_this(),
		boost::asio::placeholders::error,
		boost::asio::placeholders::bytes_transferred
	));
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
	ssl::context &ssl_ctx
)
	: io_context(io_ctx), ssl_context(ssl_ctx), acceptor(io_context)
{
}

void tls_server::start_accept()
{
	tls_connection::pointer_t new_connection = tls_connection::create(io_context, ssl_context, nullptr);
	
	acceptor.async_accept(new_connection->socket(), boost::bind(
		&tls_server::handle_accept, this, new_connection,
		boost::asio::placeholders::error
	));
}

void tls_server::handle_accept(
	tls_connection::pointer_t new_connection,
	const boost::system::error_code &err
)
{	
	if (err)
	{
		LOG(NOTICE) << endpoint_to_string(new_connection->stream().lowest_layer())
			<<"async_accept başarısız: " << err.message();
		return;
	}

	LOG(INFO) << "Yeni istemci: "
		<< endpoint_to_string(new_connection->stream().lowest_layer())
		//<< new_connection->socket().remote_endpoint().address().to_string()
		<< " SSL Handshake bekleniyor...";
	
	new_connection->start();
	start_accept();
}
