#pragma once
#include <boost/shared_ptr.hpp>
#include "util/tls_server.hpp"

namespace fuproxy
{
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
			virtual void packet_in(connection_events<void>::buffer_t&) = 0; //Başarılı bir şekilde gönderildi
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
		class event_table : public tls_connection_events<boost::shared_ptr<tls_connection>>
		{
			typedef boost::shared_ptr<tls_connection> connection_t;
		public:
			event_table(tunnel_entry *const parent, entry_target *const ent_target);

			void connect(const connection_t &);
			void read(
				const connection_t &,
				connection_events<void>::buffer_t&,
				const boost::system::error_code &,
				size_t
			);
			void write_done(
				const connection_t &,
				const boost::system::error_code &,
				size_t
			);
			void handshake(const connection_t &);
		
		private:
			tunnel_entry *const entry_parent;
			entry_target *const target;
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
		tunnel_exit();
		~tunnel_exit();
	
		/**
		 * @brief Yeni TCP Bağlantısı oluştur
		 * @details DİKKAT: SSL el sıkışmasını fonksiyonu çağıran kişini yapması gerekiyor
		 * @param host Bağlanılacak yerin IP adresi veya domaini
		 * @param port Bağlanlılacak port
		 * @param cb_table Olaylar için Callback Table
		*/
		boost::shared_ptr<tls_connection>& connect_async(
			const std::string &host,
			unsigned short port,
			tls_connection::callback_table_t *const cb_table
		);
		//void write_async(const boost::shared_ptr<tls_connection> &conn);

	private:
	};
}