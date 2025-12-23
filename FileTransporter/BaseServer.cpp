#include "BaseServer.h"


ft::BaseServer::BaseServer(const uint16_t port)
	: port(port), asioAcceptor(context)
{

}

ft::BaseServer::~BaseServer()
{
	Stop();
}

bool ft::BaseServer::Start()
{
	try
	{

		// if server restarted, restart context
		context.restart();

		if (!asioAcceptor.is_open())
		{
			asio::ip::tcp::endpoint ep(asio::ip::tcp::v4(), port);

			asioAcceptor.open(ep.protocol());
			asioAcceptor.set_option(asio::socket_base::reuse_address(true));
			asioAcceptor.bind(ep);
			asioAcceptor.listen();

		}


		WaitForClientConnection();
		thr_context = std::thread([this]() {context.run(); });

		if (IsCheckConnectionEnabled)
		{
			thr_checkConnections = std::thread([this]() {CheckConnections(); });
		}

	}
	catch (const std::exception& e)
	{
		// error when launch server
		// ...
	}


	IsRunning = true;
	OnServerStarted(); // call virtual method

	return true;
}

void ft::BaseServer::Stop()
{
	IsRunning = false;

	try
	{
		asioAcceptor.close();
	}
	catch (const std::exception& e)
	{
		// error when asioAccepto close
		// ...
	}

	
	// disconnect all clients
	for (auto& client : deqConnections)
	{
		if (client && client->IsConnected())
		{
			OnClientDisconnect(client);
			client->Disconnect();
		}
	}

	// stop context
	context.stop();

	// stop thread
	if (thr_context.joinable())
	{
		thr_context.join();
	}

	// delete pointers
	for (auto& client : deqConnections)
	{
		if (client)
		{
			client.reset();
		}
	}
	deqConnections.erase(std::remove(deqConnections.begin(), deqConnections.end(), nullptr), deqConnections.end());

	
	// call virtual method
	OnServerStopped();

}

void ft::BaseServer::WaitForClientConnection()
{
	asioAcceptor.async_accept(
		[this](std::error_code ec, asio::ip::tcp::socket socket)
		{
			if (!ec)
			{

				// new connection (client)
				std::shared_ptr<ft::Connection> newconn =
					std::make_shared<ft::Connection>(
						ft::Connection::OwnerType::Server, context, std::move(socket), queue_msg_in);

				if (OnClientConnect(newconn))
				{
					// connection accepted
					deqConnections.push_back(std::move(newconn));
					deqConnections.back()->ConnectToClient(this, clientID_counter++);
					std::cout << "[" << deqConnections.back()->GetID() << "] Connection Approved\n";
				}
				else
				{
					// connection denied
					// ...
				}
			}
			else
			{
				// new connection error
				// ...
			}

			// wait for next connection
			WaitForClientConnection();

		});
}

void ft::BaseServer::CheckConnections()
{

	while (IsRunning)
	{

		if (IsCheckConnectionEnabled)
		{
			std::this_thread::sleep_for(period_checkConnection);
		}

		bool IsInvalidClientExists = false;

		for (auto& client : deqConnections)
		{
			if (!(client && client->IsConnected()))
			{
				// disconnect client
				OnClientDisconnect(client);
				client.reset();
				IsInvalidClientExists = true;
			}
		}

		if (IsInvalidClientExists)
		{
			deqConnections.erase(std::remove(deqConnections.begin(), deqConnections.end(), nullptr), deqConnections.end());
		}


		if (!IsCheckConnectionEnabled)
		{
			break;
		}

	}
}

void ft::BaseServer::DisconnectAllClients()
{
	for (auto& client : deqConnections)
	{
		if (client && client->IsConnected())
		{
			client->Disconnect();
			client.reset();
		}
	}

	deqConnections.erase(std::remove(deqConnections.begin(), deqConnections.end(), nullptr), deqConnections.end());
}

bool ft::BaseServer::MessageClient(std::shared_ptr<ft::Connection> client, const ft::Message& msg)
{
	if (client->IsConnected())
	{
		client->Send(msg);
		return true;
	}
	else if (!client)
	{
		return false;
	}
	else
	{
		OnClientDisconnect(client);
		client.reset();
		deqConnections.erase(std::remove(deqConnections.begin(), deqConnections.end(), client), deqConnections.end());
		return false;
	}
}

void ft::BaseServer::Update(size_t nMax_Messages, bool isWait)
{
	if (isWait)
	{
		queue_msg_in.wait();
	}

	size_t nMessageCount = 0;
	while (nMessageCount < nMax_Messages && !queue_msg_in.empty())
	{
		auto owned_message = queue_msg_in.pop_front();
		OnMessage(owned_message.remote, owned_message.msg);
		nMessageCount++;
	}
}
