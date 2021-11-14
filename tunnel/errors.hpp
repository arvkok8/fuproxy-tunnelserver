#pragma once
#include <system_error>

namespace fuproxy
{
	enum class tunnel_errors
	{
		no_such_token = 10,
		expired_token,
		timeout
	};

}