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


