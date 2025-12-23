#pragma once
#include "AsioInclude.h"
#include "Connection.h"
#include "Message.h"


namespace ft
{

	class BaseClient
	{

	public:
		BaseClient();
		virtual ~BaseClient();

		void Connect(const std::string& server_ip, const uint16_t server_port, std::function<void(bool success)> on_complete);
		void Disconnect();
		bool IsConnected();

		ft::TSQueue<ft::OwnedMessage>& get_IncomingMsg() { return queue_msg_in; }

		void Send(ft::Message& msg) { connection->Send(msg); }

	protected:

		asio::io_context context;
		std::thread thr_context;
		asio::ip::tcp::socket socket;

		std::shared_ptr<ft::Connection> connection;


	private:

		ft::TSQueue<ft::OwnedMessage> queue_msg_in;

	};

}

