#pragma once
#include <boost/asio.hpp>
#include <vector>
//#include "tls_server.hpp"

//Circular dependency olayını engellemek için template kullanmam gerekti
template <typename source_t>
class connection_events
{
public:
	typedef boost::asio::dynamic_vector_buffer<uint8_t, std::vector<uint8_t>::allocator_type> buffer_t;

	virtual ~connection_events(){}

 	/**
	  * @brief Bağlantı başarılı bir şekilde kuruldu
	  * @param conn Olayın gerçekleştiği tls_connection nesnesi
	 */
	virtual void connect(const source_t &conn) = 0;
	
	/**
	 * @brief Soket okuma modunda beklerken bir olay gerçekleşti
	 * @param conn Olayın gerçekleştiği tls_connection nesnesi
	 * @param buffer Gelen veri için buffer
	 * @param err Gerçekleşen hata (Success olabilir)
	 * @param len Buffera kaç byte ver yazıldı
	*/
	virtual void read(
		const source_t &conn,
		buffer_t &buffer,
		const boost::system::error_code &err,
		size_t len
	) = 0;
	
	/**
	 * @brief write_async fonksiyonun sonucu geldi
	 * @param conn Olayın gerçekleştiği connection nesnesi
	 * @param err Gerçekleşen hata (Success olabilir)
	 * @param len Sokete kaç byte veri yazıldı
	*/
	virtual void write_done(
		const source_t &conn,
		const boost::system::error_code &err,
		size_t len
	) = 0;

};

template <typename source_t>
class tls_connection_events : public connection_events<source_t>
{
public:
	virtual ~tls_connection_events(){}

	virtual void handshake(const source_t &conn) = 0; //TLS Handshake sonucu geldi
};