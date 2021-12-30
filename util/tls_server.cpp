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

void tls_connection::async_connect(ip::tcp::resolver::results_type endpoints)
{
	LOG_DEBUG("tls_connection[" << static_cast<void*>(this) << "] async_connect: "
		<< endpoints.begin()->endpoint().address().to_string()
		<< " adresine bağlanmayı deniyor..."
	);
	boost::asio::async_connect(
		secure_stream.next_layer(),
		endpoints,
		boost::bind(
			&tls_connection::handle_connect,
			shared_from_this(),
			boost::asio::placeholders::error,
			boost::asio::placeholders::endpoint
		)
	);
}

void tls_connection::async_connect_unsecure(ip::tcp::resolver::results_type endpoints)
{
	LOG_DEBUG("tls_connection[" << static_cast<void*>(this) << "] async_connect_unsecure: "
		<< endpoints.begin()->endpoint().address().to_string()
		<< " adresine bağlanmayı deniyor..."
	);

	boost::asio::async_connect(
		secure_stream.next_layer(),
		endpoints,
		boost::bind(
			&tls_connection::handle_connect_unsecure,
			shared_from_this(),
			boost::asio::placeholders::error,
			boost::asio::placeholders::endpoint
		)
	);
}

void tls_connection::handle_connect(
	const boost::system::error_code &err,
	const boost::asio::ip::tcp::endpoint &endpoint
)
{
	if(err)
	{
		LOG_DEBUG("tls_connection[" << static_cast<void*>(this)  << "] handle_connect: "
			<< endpoint.address().to_string()
			<< " adresine bağlanılamadı: " << err.message());
		
		///TODO: projeye başlarken plan yapmamanın sonuçları. bu yöntemin sadece tunnel_exit için
		///      kullanılacağını umuyorum inş başka birşeyle çakışmaz
		callback_table->connect(nullptr);
		
		return;
	}

	LOG_DEBUG("tls_connection[" << static_cast<void*>(this)  << "] handle_connect: "
			<< endpoint.address().to_string() << " adresine bağlanıldı");

	this->start_handshake_as_client();
}

void tls_connection::handle_connect_unsecure(
	const boost::system::error_code &err,
	const boost::asio::ip::tcp::endpoint &endpoint
)
{
	if(err)
	{
		LOG_DEBUG("tls_connection[" << static_cast<void*>(this)  << "] handle_connect_unsecure: "
			<< endpoint.address().to_string()
			<< " adresine bağlanılamadı: " << err.message());
		
		///TODO: projeye başlarken plan yapmamanın sonuçları. bu yöntemin sadece tunnel_exit için
		///      kullanılacağını umuyorum inş başka birşeyle çakışmaz
		callback_table->connect(nullptr);
		
		return;
	}

	LOG_DEBUG("tls_connection[" << static_cast<void*>(this)  << "] handle_connect_unsecure: "
			<< endpoint.address().to_string() << " adresine bağlanıldı");

	callback_table->connect(shared_from_this());
}

void tls_connection::start_handshake_as_server()
{
	if (callback_table) callback_table->connect(shared_from_this()); 

	LOG_DEBUG("tls_connection[" << static_cast<void*>(this) << "] start_handshake_as_server: "
			<< endpoint_to_string(this->socket())
			<< " SSL Handshake bekliyor...");

	secure_stream.async_handshake(
		ssl::stream_base::server,
		boost::bind(
			&tls_connection::handle_handshake,
			shared_from_this(),
			boost::asio::placeholders::error
		)
	);
}

void tls_connection::start_handshake_as_client()
{
	if (callback_table) callback_table->connect(shared_from_this()); 

	LOG_DEBUG("tls_connection[" << static_cast<void*>(this) << "] start_handshake_as_client: "
			<< endpoint_to_string(this->socket())
			<< " SSL Handshake gönderildi...");

	secure_stream.async_handshake(
		ssl::stream_base::client,
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
		LOG_DEBUG("tls_connection[" << static_cast<void*>(this) << "] handle_handshake: "
			<< endpoint_to_string(secure_stream.lowest_layer())
			<< " İstemci ile SSL Handshake başarısız, bağlantı kapatılıyor: " << err.message());
		return;
	}

	LOG_DEBUG("tls_connection[" << static_cast<void*>(this) << "] handle_shake: "
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

void tls_connection::async_read_some_unsecure()
{
	boost::asio::async_read(
		secure_stream.next_layer(),
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
	secure_stream.async_shutdown([&](const boost::system::error_code &){
		secure_stream.lowest_layer().close();
	});
	//secure_stream.lowest_layer().close();
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
	port(listen_port),
	callback_table(cb_table)
{
	start_accept();
}

void tls_server::start_accept()
{
	tls_connection::pointer_t new_connection = tls_connection::create(io_context, ssl_context, callback_table);

	LOG_DEBUG("tls_server: " << port << " portunda dinlemeye başladı...");

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
		LOG_DEBUG("tls_server: IP " << endpoint_to_string(new_connection->stream().lowest_layer())
			<< "ile async_accept başarısız: " << err.message());
		return;
	}

	LOG_DEBUG("tls_server: "
		<< endpoint_to_string(new_connection->stream().lowest_layer())
		<< " SSL Handshake bekleniyor...");
	
	new_connection->start_handshake_as_server();
	start_accept();
}
