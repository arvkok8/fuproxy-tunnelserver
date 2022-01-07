#pragma once

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/array.hpp>
#include <boost/bind/bind.hpp>
#include "util/connection_events.hpp"

class tls_connection : public boost::enable_shared_from_this<tls_connection>
{
public:
	typedef boost::shared_ptr<tls_connection> pointer_t;
	typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> stream_t;
	typedef tls_connection_events callback_table_t;

	/**
	 * @brief Yeni bir SSL Stream oluştur
	 * @param io boost::asio::io_context nesnesi
	 * @param ssl boost::...::ssl::context nesnesi
	 * @param callback_table Olacak olaylar için callback tablosu. Null verilebilir
	*/
	static pointer_t create(
		boost::asio::io_context &io,
		boost::asio::ssl::context &ssl,
		callback_table_t *const callback_table
	);
	stream_t& stream();
	stream_t::next_layer_type& socket();

	void async_connect(boost::asio::ip::tcp::resolver::results_type endpoints);
	void async_connect_unsecure(boost::asio::ip::tcp::resolver::results_type endpoints);

	/**
	 * @brief SSL Handshake işlemini başlat
	*/
	void start_handshake_as_server();

	void start_handshake_as_client();
	
	/**
	 * @brief Asenkron olarak verilen veriyi yaz
	 * @tparam buffer_t boost::asio tarafından okunabilecek buffer nesnesi
	*/
	template <typename buffer_t>
	void async_write(const buffer_t &buf)
	{
		boost::asio::async_write(
			secure_stream, 
			boost::asio::buffer(buf),
			boost::asio::transfer_all(),
			boost::bind(
				&tls_connection::handle_write, shared_from_this(),
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred
			)
		);
	}

	template <typename buffer_t>
	void async_write_unsecure(const buffer_t &buf)
	{
		boost::asio::async_write(
			secure_stream.next_layer(), 
			boost::asio::buffer(buf),
			boost::asio::transfer_all(),
			boost::bind(
				&tls_connection::handle_write, shared_from_this(),
				boost::asio::placeholders::error,
				boost::asio::placeholders::bytes_transferred
			)
		);
	}

	/**
	 * @brief Asenkron olarak verilen veriyi yaz ve veri ulaşır ulaşmaz bağlantıyı kapat
	 * @tparam buffer_t boost::asio tarafından okunabilecek buffer nesnesi
	*/
	template <typename buffer_t>
	void async_write_error(const buffer_t &buf)
	{
		auto thisptr = shared_from_this();

		boost::asio::async_write(
			secure_stream, 
			boost::asio::buffer(buf),
			boost::asio::transfer_all(),
			[=](
				const boost::system::error_code&,
				size_t
			)
			{
				thisptr->disconnect();
			}
		);
	}

	void async_read_some();
	void async_read_some_unsecure();
	void disconnect();

private:
	tls_connection(boost::asio::io_context&, boost::asio::ssl::context&, callback_table_t *const);

	void handle_connect(
		const boost::system::error_code&,
		const boost::asio::ip::tcp::endpoint&
	);
	void handle_connect_unsecure(
		const boost::system::error_code&,
		const boost::asio::ip::tcp::endpoint&
	);
	void handle_handshake(const boost::system::error_code&);
	void handle_read(const boost::system::error_code&, size_t);
	void handle_write(const boost::system::error_code&, size_t);

	stream_t secure_stream;
	callback_table_t *const callback_table;
	std::vector<uint8_t> read_buffer;
	connection_events::buffer_t read_buffer_view;
	
};

class tls_server
{
public:
	tls_server(
		unsigned short listen_port,
		boost::asio::io_context&,
		boost::asio::ssl::context&,
		tls_connection::callback_table_t *const
	);
	
	void start_accept();
	void stop_accept();

private:

	void handle_accept(const tls_connection::pointer_t &new_connection, const boost::system::error_code &err);
	
	boost::asio::io_context &io_context;
	boost::asio::ssl::context &ssl_context;
	boost::asio::ip::tcp::acceptor acceptor;
	unsigned short port;
	tls_connection::callback_table_t *const callback_table;
};