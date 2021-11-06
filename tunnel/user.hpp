#pragma once
#include <string>
#include <boost/make_unique.hpp>

#include "tunnel/connection_token.hpp"

namespace fuproxy
{
	class user
	{
	public:
		user();
		user();
		~user();

		static user from_json(const std::string&);

	private:
		std::string username;
		connection_token *token;
		uint64_t expiry_date; //Token en son ne zamana kadar kullanılabilir
	};

}