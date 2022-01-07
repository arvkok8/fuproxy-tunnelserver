#pragma once
#include <string>
#include <sstream>
#include <boost/asio.hpp>
#include <boost/json.hpp>

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
		<< socket.remote_endpoint().address()
		<< ":"
		<< socket.remote_endpoint().port();

	return sstream.str();
}

std::string generate_response_json(const std::function<std::string(const std::string&)>& func);
std::string generate_error_json(const std::string &cmd, const boost::json::object &response);
std::string generate_success_json(const std::string &cmd, const boost::json::object &response);