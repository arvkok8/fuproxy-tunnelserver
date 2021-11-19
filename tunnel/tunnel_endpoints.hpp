#pragma once
#include <boost/shared_ptr.hpp>
#include <unordered_map>
#include <utility>
#include <mutex>
#include <array>
#include "util/tls_server.hpp"
#include "tunnel/tunnel_route_information.hpp"

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
		typedef std::array<uint8_t, 64> connection_token_pod_t;

		tunnel_exit(
			boost::asio::io_context&,
			boost::asio::ssl::context&
		);
		~tunnel_exit();
	
		/**
		 * @brief Yeni SSL Bağlantısı oluştur
		 * @details DİKKAT: SSL el sıkışmasını fonksiyonu çağıran kişini yapması gerekiyor
		 * @param from Hangi IP adresinden geliyor. Saat çok geç ve ne yaptığımı bilmiyorum, değişebilir
		 * @param host Bağlanılacak yerin IP adresi veya domaini
		 * @param port Bağlanlılacak port
		 * @param result_cb Sonucun verileceği fonksiyon
		*/
		void async_connect_secure(
			boost::asio::ip::tcp::endpoint from,
			const std::string &host,
			unsigned short port,
			std::function<void(tunnel_route_information::connection_result_t)> result_cb
		);

		/**
		 * @brief Yeni TCP Bağlantısı oluştur
		 * @details Daha sonradan tekrar SSLe yükseltebiliriz ama muhtemelen gerek kalmayacak
		 * @param from Hangi IP adresinden geliyor. Saat çok geç ve ne yaptığımı bilmiyorum, değişebilir
		 * @param host Bağlanılacak yerin IP adresi veya domaini
		 * @param port Bağlanlılacak port
		 * @param result_cb Sonucun verileceği fonksiyon
		*/
		void async_connect_unsecure(
			boost::asio::ip::tcp::endpoint from,
			const std::string &host,
			unsigned short port,
			std::function<void(tunnel_route_information::connection_result_t)> result_cb
		);

		//TODO: TLS olmadan bağlanmak için fonksiyonlar ekle

		static connection_token_pod_t generate_connection_token_pod();
		static std::string generate_connection_token();
		static std::string generate_connection_token(const connection_token_pod_t&);

		boost::asio::io_context &io_context;
		boost::asio::ssl::context &ssl_context;

	private:
		class event_table : public tls_connection_events
		{
		public:
			friend class tunnel_exit;

			event_table(tunnel_exit&);
			~event_table();

			void connect(event_table::source_t);
			void handshake(event_table::source_t);
			void read(
				event_table::source_t,
				event_table::buffer_t&,
				const boost::system::error_code&,
				size_t
			);
			void write_done(
				event_table::source_t,
				const boost::system::error_code&,
				size_t
			);

		private:
			tunnel_exit &exit_parent;
		};

		typedef std::unordered_map<tls_connection_events::source_t, tunnel_route_information> connection_list_t;
		
		connection_list_t connection_list;

		event_table ev_table;
	};
}