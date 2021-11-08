#pragma once
#include <string>
#include <vector>

namespace fuproxy
{
	class basic_token
	{
	public:
		virtual ~basic_token() {}

		/**
		 * @brief İnsan tarafından okunabilir token. Debug dışında pek bir kullanımı olmayabilir
		*/
		virtual std::string to_string() = 0;

		/**
		 * @brief Token'in POD hali
		*/
		virtual std::vector<uint8_t> to_data() = 0;

		/**
		 * @brief Token nesnesini POD ile oluştur
		*/
		virtual void from_data(std::vector<uint8_t>) = 0;

		/**
		 * @brief Token'in ne zaman geçersiz olacağını getir
		 * @return UNIX Timestamp
		*/
		virtual uint64_t get_expiry() = 0;

	private:
	};
}