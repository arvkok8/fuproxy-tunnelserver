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
		
		std::vector<uint8_t> to_data();

		void from_data(std::vector<uint8_t>);

	private:
		boost::uuids::uuid data;
	};
}