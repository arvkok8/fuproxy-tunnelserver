#pragma once
#include <boost/shared_ptr.hpp>
#include <unordered_map>
#include <mutex>
#include "util/tls_server.hpp"

namespace fuproxy
{
	struct universal_header
	{
		static const uint32_t SIGN = 0x80542013; //Kesinlikle okul numarası değil
		uint32_t signature;
		uint16_t len;
	} __attribute__((packed));

	struct connection_stats
	{
		bool receiving; //Paketin tamamlanmasını bekliyor muyuz?
		size_t received; //Şuana kadar kaç byte geldi
		size_t to_be_received; //Kaç byte bekleniyor
	};

	/**
	 * TCP Acceptor Wrapper'i
	 * connection_events için router sınıfının verdiği fonksiyonlar kullanılacak
	*/
	class tunnel_entry
	{
	public:
		class entry_target
		{
		public:
			virtual void packet_in(
				connection_events::source_t,
				connection_events::buffer_t&
			) = 0; //Başarılı bir şekilde gönderildi
			virtual void packet_out() = 0; //Gönderilemedi
		};

		tunnel_entry(
			unsigned short port,
			boost::asio::io_context&,
			boost::asio::ssl::context&,
			entry_target *const
		);
		~tunnel_entry();

		void start_listen();

	private:
		class event_table : public tls_connection_events
		{
		public:
			event_table(tunnel_entry *const parent, entry_target *const ent_target);

			void connect(source_t );
			void read(
				source_t,
				connection_events::buffer_t&,
				const boost::system::error_code &,
				size_t
			);
			void write_done(
				source_t,
				const boost::system::error_code &,
				size_t
			);
			void handshake(source_t);
		
		private:
			tunnel_entry *const entry_parent;
			entry_target *const target;
			std::unordered_map<source_t, connection_stats> stats_table;
			std::mutex stats_write;
		};
		event_table ev_table;
		entry_target *const target;
		tls_server server;
	};

	/**
	 * "connect" ve benzeri komutlar için wrapper
	 * tls_connection sınıfı bu sınıf ile etkileşmeyecek
	*/
	class tunnel_exit
	{
	public:
		tunnel_exit(
			boost::asio::io_context&,
			boost::asio::ssl::context&
		);
		~tunnel_exit();
	
		/**
		 * @brief Yeni SSL Bağlantısı oluştur
		 * @details DİKKAT: SSL el sıkışmasını fonksiyonu çağıran kişini yapması gerekiyor
		 * @param host Bağlanılacak yerin IP adresi veya domaini
		 * @param port Bağlanlılacak port
		 * @param cb_table Olaylar için Callback Table
		*/
		boost::shared_ptr<tls_connection> connect_secure_async(
			const std::string &host,
			unsigned short port,
			tls_connection::callback_table_t *const cb_table
		);

		//TODO: TLS olmadan bağlanmak için fonksiyonlar ekle

		boost::asio::io_context &io_context;
		boost::asio::ssl::context &ssl_context;

	private:
	};
}