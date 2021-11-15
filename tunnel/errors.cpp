#include "tunnel/errors.hpp"

using namespace fuproxy::errors;

struct tunnel_error_category : std::error_category
{
	const char* name() const noexcept override;
	std::string message(int ev) const override;
};

const char* tunnel_error_category::name() const noexcept
{
	return "tunnel";
}

std::string tunnel_error_category::message(int ev) const
{
	switch(static_cast<tunnel_errors>(ev))
	{
		case tunnel_errors::no_such_token:
			return "sorgulanan token geçerli değil";
		case tunnel_errors::expired_token:
			return "token artık geçersiz";
		case tunnel_errors::timeout:
			return "sorgu zaman aşımına uğradı";
		default:
			return "bilinmeyen hata";
	}
}

const tunnel_error_category tunnel_error_category_inst {};

std::error_code fuproxy::errors::make_error_code(tunnel_errors e)
{
	return  {static_cast<int>(e), tunnel_error_category_inst};
}
