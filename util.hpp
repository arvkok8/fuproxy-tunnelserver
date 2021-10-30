#include <string>
#include <sstream>
#include <boost/asio.hpp>

/**
 * @brief SSL Stream nesnesinden SRCPORT:DSTADDR:DSTPORT formatında string üretir
 * @details
 *  Template P ve E değerlerini derleriyici lowest_layer() return türüne göre otomatik olarak doldurur
 * @tparam P asio::basic_socket için Protocol türü
 * @tparam E asio::basic_socket için Executor türü
 * @param socket Bağlı soket
 * @return SRCPORT:DSTADDR:DSTPORT formatında metin
*/
template <typename P, typename E>
static std::string endpoint_to_string(const boost::asio::basic_socket<P, E> &socket)
{
	std::stringstream sstream("");

	sstream
		<< socket.local_endpoint().port()
		<< ":"
		<< socket.remote_endpoint().address().to_string()
		<< ":"
		<< socket.remote_endpoint().port();

	return sstream.str();
}