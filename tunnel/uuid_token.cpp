#include "tunnel/uuid_token.hpp"
#include <boost/uuid/uuid_io.hpp>

using namespace fuproxy;
namespace uuids = boost::uuids;

uuid_token::uuid_token()
	: data(uuids::nil_uuid()) {}

uuid_token::uuid_token(const uuids::uuid &u)
	: data(u) {}

uuid_token::uuid_token(const uuid_token &u)
	: data(u.data) {}

uuid_token::~uuid_token() {}

std::string uuid_token::to_string()
{
	return uuids::to_string(data);
}

std::vector<uint8_t> uuid_token::to_data()
{
	std::vector<uint8_t> ret(sizeof(uint8_t) * 16);
	ret.assign(data.data, data.data + 16);
	return ret;
}

void uuid_token::from_data(std::vector<uint8_t> v)
{
	//! Boundary check yapılmalı
	std::copy(v.begin(), v.end(), data.data);
}
