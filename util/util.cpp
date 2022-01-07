#include "util.hpp"
#include <boost/json/src.hpp>
#include <boost/property_tree/ptree.hpp>

namespace json = boost::json;

/**
 * Template olmayan fonksiyonlar için tanımlar burada bulunacak
*/

/**
 * @brief İstemciye göndermek için JSON yanıt mesajı oluştur
 * @param func Paket kökünün her elemanı için çağırılacak fonksyion
 * @return Oluşturulan yanıt JSON metini
*/
std::string generate_response_json(const std::function<std::string(const std::string&)>& func)
{
	std::stringstream ss;

	ss << "{";

	ss << "\"success\":" << func("success") << ",";
	ss << "\"command\":" << func("command") << ",";
	ss << "\"command_args\":" << func("command_args") << ",";
	ss << "\"response\":" << func("response");

	ss << "}";

	return ss.str();
}

std::string generate_error_json(const std::string &cmd, const json::object &response)
{
	return generate_response_json(
		[&](const std::string &param) -> std::string
		{
			if(param == "success") return "false";
			if(param == "command") return std::string("\"") + cmd + "\"";
			if(param == "response") return json::serialize(response);
			return "null";
		}
	);
}

std::string generate_success_json(const std::string &cmd, const json::object &response)
{
	return generate_response_json(
		[&](const std::string &param) -> std::string
		{
			if(param == "success") return "true";
			if(param == "command") return std::string("\"") + cmd + "\"";
			if(param == "response") return json::serialize(response);
			return "null";
		}
	);
}