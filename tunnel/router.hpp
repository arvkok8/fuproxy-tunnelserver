#pragma once
#include "tunnel/tunnel_endpoints.hpp"
/**
 * Gelen paketlerin nereye gideceğini belirleyecek class.
 * Sadece headerlar ile çalışacak payload ile bir işi yok
*/

namespace fuproxy
{
	class router : public fuproxy::tunnel_entry::entry_target
	{
	public:
		router(tunnel_exit *const);
		~router();

		void packet_in(
			connection_events::source_t,
			connection_events::buffer_t &data
		);
		void packet_out();

	private:
		tunnel_exit *const exit;
	};
}