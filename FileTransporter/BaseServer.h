#pragma once
#include "AsioInclude.h"
#include "Connection.h"
#include "Message.h"


namespace ft
{

	class BaseServer
	{

	public:
		BaseServer(const uint16_t port);
		virtual ~BaseServer();

		bool Start();
		void Stop();
		void WaitForClientConnection();

		// send msg to client
		bool MessageClient(std::shared_ptr<ft::Connection> client, const ft::Message& msg);

		void Update(size_t nMax_Messages = -1, bool isWait = false);

		void SetCheckConncectionEnable(bool enable) { IsCheckConnectionEnabled = enable; }
		void SetPeriodCheckConnection(uint32_t seconds) { period_checkConnection = std::chrono::seconds(seconds); }

		ft::TSQueue<ft::OwnedMessage>& get_IncomingMsg() { return queue_msg_in; }


	protected:

		uint16_t port;

		// incoming messages
		ft::TSQueue<ft::OwnedMessage> queue_msg_in;

		// active connections (clients)
		std::deque<std::shared_ptr<ft::Connection>> deqConnections;

		asio::io_context context;
		std::thread thr_context;

		// listener. initialize in constructor with context and ip_adress
		asio::ip::tcp::acceptor asioAcceptor;

		uint32_t clientID_counter = 1000;

		ft::Message temprorary_msg;

		std::thread thr_checkConnections;
		std::chrono::seconds period_checkConnection = std::chrono::seconds(5); // by default 5 seconds
		bool IsCheckConnectionEnabled = true;

		bool IsRunning = false;

	protected:

		void CheckConnections();

		void DisconnectAllClients();


		// called when client connected. we can reject client by returning false
		virtual bool OnClientConnect(std::shared_ptr<ft::Connection> client) { return false; }

		// called when client disconnected
		virtual void OnClientDisconnect(std::shared_ptr<ft::Connection> client) {}

		// called when message arrives
		virtual void OnMessage(std::shared_ptr<ft::Connection> client, const ft::Message& msg) {}

		virtual void OnServerStarted() {}
		virtual void OnServerStopped() {}

	public:
		// called when client succeeded validation
		virtual void OnClientValidated(std::shared_ptr<ft::Connection> client) {}

	};

}

