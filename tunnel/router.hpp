#pragma once
#include "tunnel/tunnel_endpoints.hpp"
#include <tunnel/authenticator.hpp>
#include <boost/property_tree/ptree.hpp>
#include <unordered_set>

/**
 * Gelen paketlerin nereye gideceğini belirleyecek class.
 * Sadece headerlar ile çalışacak payload ile bir işi yok
*/

namespace fuproxy
{
	class router : public fuproxy::tunnel_entry::entry_target
	{
	public:
		router(tunnel_exit *const, authenticator *const);
		~router();

		void packet_in(
			connection_events::source_t,
			connection_events::buffer_t &data
		);
		void packet_out();

	private:
		tunnel_exit *const exit;
		authenticator *const auth;

		void handle_start_tunnel(connection_events::source_t, boost::property_tree::ptree tree);
		void handle_connect(connection_events::source_t, boost::property_tree::ptree tree);
		void handle_connect_response();
		void handle_data(connection_events::source_t, boost::property_tree::ptree tree);
	};
}