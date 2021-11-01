#pragma once

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/array.hpp>
#include "connection_events.hpp"

class tls_connection : public boost::enable_shared_from_this<tls_connection>
{
public:
	typedef boost::shared_ptr<tls_connection> pointer_t;
	typedef boost::asio::ssl::stream<boost::asio::ip::tcp::socket> stream_t;
	typedef boost::shared_ptr<tls_connection_events_i< boost::shared_ptr<tls_connection> >> callback_table_t;

	/**
	 * @brief Yeni bir SSL Stream oluştur
	 * @param io boost::asio::io_context nesnesi
	 * @param ssl boost::...::ssl::context nesnesi
	 * @param callback_table Olacak olaylar için callback tablosu. Null verilebilir
	*/
	static pointer_t create(
		boost::asio::io_context &io,
		boost::asio::ssl::context &ssl,
		const callback_table_t &callback_table
	);
	stream_t& stream();
	stream_t::next_layer_type& socket();

	/**
	 * @brief SSL Handshake işlemini başlat
	*/
	void start();
	
	/**
	 * @brief Asenkron olarak verilen veriyi yaz
	 * @tparam buffer_t boost::asio tarafından okunabilecek buffer nesnesi
	*/
	template <typename buffer_t>
	void write_async(const buffer_t &buf);

	/**
	 * @brief Soketten okumaya başla
	*/
	void start_read();
	void disconnect();

private:
	tls_connection(boost::asio::io_context&, boost::asio::ssl::context&, const callback_table_t&);

	void handle_handshake(const boost::system::error_code&);
	void handle_read(const boost::system::error_code&, size_t);
	void handle_write(const boost::system::error_code&, size_t);

	stream_t secure_stream;
	const callback_table_t &callback_table;
	boost::array<char, 1024> read_buffer;
};

class tls_server
{
public:
	tls_server(unsigned short listen_port, boost::asio::io_context&, boost::asio::ssl::context&);
	
	void start_accept();
	void stop_accept();

private:

	void handle_accept(const tls_connection::pointer_t &new_connection, const boost::system::error_code &err);
	
	boost::asio::io_context &io_context;
	boost::asio::ssl::context &ssl_context;
	boost::asio::ip::tcp::acceptor acceptor;
	unsigned short port;
};