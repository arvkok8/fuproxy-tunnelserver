#pragma once
#include "tunnel/basic_token.hpp"
#include <boost/uuid/uuid_generators.hpp>

namespace fuproxy
{
	class uuid_token : public basic_token
	{
	public:
		uuid_token();
		uuid_token(const boost::uuids::uuid&);
		uuid_token(const uuid_token&);
		~uuid_token();

		std::string to_string();
		
		std::vector<uint8_t> get_data();
		void set_data(std::vector<uint8_t>);
		
		time_t get_expiry();
		void set_expiry(time_t);
		
	private:
		boost::uuids::uuid data;
		time_t expiry;
	};
}