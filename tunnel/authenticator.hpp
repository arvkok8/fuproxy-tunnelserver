#pragma once
#include "tunnel/user.hpp"

#include <boost/function.hpp>

namespace fuproxy
{
	class authenticator
	{
	public:
		authenticator();
		~authenticator();

		void query_user(const fuproxy::user&);
		void query_user_async(const boost::function<void(const fuproxy::user&)> &f);

		/**
		 * @brief Kullanıcının son aktivite değerini güncelle
		*/
		void update_user(const fuproxy::user&);
	};
}
