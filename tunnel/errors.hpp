#pragma once
#include <system_error>

namespace fuproxy
{
namespace errors
{
		enum class tunnel_errors
		{
			no_such_token = 10,
			expired_token,
			timeout
		};

		std::error_code make_error_code(tunnel_errors e);
}
}

namespace std
{
	template <>
	struct is_error_code_enum<fuproxy::errors::tunnel_errors> : true_type
	{};
}