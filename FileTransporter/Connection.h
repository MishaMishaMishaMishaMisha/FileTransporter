#pragma once

#include "AsioInclude.h"
#include "Message.h"
#include <string>
#include <fstream>
#include <iostream>
#include <vector>
#include "ThreadsafeQueue.h"
#include "FileTransferState.h"


namespace ft
{

	class BaseServer;

	class Connection : public std::enable_shared_from_this<Connection>
	{
	
	public:

		enum class OwnerType {Server, Client};

		Connection(OwnerType owner, asio::io_context& context, asio::ip::tcp::socket socket, ft::TSQueue<ft::OwnedMessage>& q_in);

		virtual ~Connection();

		void ConnectToClient(ft::BaseServer* server, uint32_t uid = 0);

		void ConnectToServer(const asio::ip::tcp::resolver::results_type& serverInfo, std::function<void(bool success)> on_complete);

		void Disconnect();

		bool IsConnected() const;

		uint32_t GetID() const { return id; }

		void Send(const ft::Message& msg);

		std::function<void()> onConnectionLost;


	public:

		ft::FileTransferState current_file;

	protected:

		asio::ip::tcp::socket socket;

		asio::io_context& context;

		OwnerType owner = OwnerType::Server;

		ft::TSQueue<ft::OwnedMessage>& queue_msg_in; // reference of incoming messages. set in owner of connection
		ft::TSQueue<ft::Message> queue_msg_out; // messages for sending

		uint32_t id = 0; // client id

		ft::Message temproraryMsgIn;
		ft::Message temproraryMsgOut;

		// validating
		uint64_t val_data_in = 0;
		uint64_t val_data_out = 0;
		uint64_t val_data_check = 0;

	private:
		// async methods write/read
		void ReadHeader();
		void ReadBody();
		void WriteHeader();
		void WriteBody();
		
		void AddToIncomingMsgQueue();


		// validating
		uint64_t encryptValidatingData(uint64_t inputData);
		void WriteValidation();
		void ReadValidation(ft::BaseServer* server = nullptr);

	};

}

