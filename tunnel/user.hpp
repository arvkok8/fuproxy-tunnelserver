#pragma once
#include <string>
#include <boost/shared_ptr.hpp>

#include "tunnel/basic_token.hpp"

namespace fuproxy
{
	class user
	{
	public:
		user();
		user(const user&);
		~user();

		static user from_json(const std::string&);

	private:
		std::string username;
		std::shared_ptr<basic_token> user_token;
		uint64_t expiry_date; //Token en son ne zamana kadar kullanılabilir
	};

}