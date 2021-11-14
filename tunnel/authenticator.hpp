#pragma once
#include "tunnel/user.hpp"
#include <system_error>

#include <functional>

namespace fuproxy
{
	class authenticator
	{
	public:
		authenticator();
		~authenticator();

		void query_user(const fuproxy::user&);
		void query_user_async(const std::function<void(const fuproxy::user&)> &handler);

		/**
		 * @brief Kullanıcı token'ini sorgular ve uygun son kullanma tarihini girer
		 * @details
		 * 	Handler fonksiyonu parametreleri sırasıyla
		 * 	1: async_query_user_token çağırılırken kullanılan token değeri
		 * 	2: Sorgulama sırasında ne hatası oluştu
		 * 	Eğer sorgu sırasında bir hata oluşmadıysa token güncellenmiştir
		 * @param token Sorgulanacak token
		 * @param handler İşlem sonucunu işleyecek fonksiyon
		*/
		void async_query_user_token(
			basic_token &token,
			const std::function<void(basic_token&, const std::error_code&)> &handler
		);

		/**
		 * @brief Kullanıcının son aktivite değerini güncelle
		*/
		void update_user(const fuproxy::user&);
	};
}
