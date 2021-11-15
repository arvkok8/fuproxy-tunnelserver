#include "tunnel/authenticator.hpp"
#include "tunnel/errors.hpp"
#include <limits>

using namespace fuproxy;

authenticator::authenticator()
{

}

authenticator::~authenticator()
{

}

void authenticator::async_query_user_token(
    basic_token *token,
    const std::function<void(basic_token*, const std::error_code&)> &handler
)
{
    token->set_expiry(std::numeric_limits<time_t>::max());
    std::error_code err = static_cast<errors::tunnel_errors>(0);
    handler(token, err);
}