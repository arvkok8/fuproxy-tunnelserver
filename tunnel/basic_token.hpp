#pragma once
#include <string>
#include <vector>

namespace fuproxy
{
	class basic_token
	{
	public:
		typedef uint64_t timestamp_t;

		virtual ~basic_token() {}

		/**
		 * @brief İnsan tarafından okunabilir token. Debug dışında pek bir kullanımı olmayabilir
		*/
		virtual std::string to_string() = 0;

		/**
		 * @brief Token'in POD hali
		*/
		virtual std::vector<uint8_t> get_data() = 0;

		/**
		 * @brief Token nesnesini POD ile oluştur
		*/
		virtual void set_data(std::vector<uint8_t>) = 0;

		/**
		 * @brief Token'in ne zaman geçersiz olacağını getir
		 * @return UNIX Timestamp
		*/
		virtual time_t get_expiry() = 0;

		/**
		 * @brief Token'in yeni son kullanma tarihini değiştir
		*/
		virtual void set_expiry(time_t) = 0;

	private:
	};
}