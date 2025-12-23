#include "BaseClient.h"


ft::BaseClient::BaseClient() : socket(context)
{

}

ft::BaseClient::~BaseClient()
{
	Disconnect();
}

void ft::BaseClient::Connect(const std::string& server_ip, const uint16_t server_port, std::function<void(bool success)> on_complete)
{

	Disconnect(); // clear old connection if reconnect

	// resolver: convert string of hostname into address that can be used for connection
	asio::ip::tcp::resolver resolver(context);
	asio::ip::tcp::resolver::results_type endpoints = resolver.resolve(server_ip, std::to_string(server_port));

	// create connection
	connection = std::make_shared<ft::Connection>(ft::Connection::OwnerType::Client, context, asio::ip::tcp::socket(context), queue_msg_in);

	connection->ConnectToServer(endpoints, on_complete);
	
	thr_context = std::thread([this]() {context.run(); });


}


void ft::BaseClient::Disconnect() 
{
	if (connection) 
	{
		// close socket
		asio::post(context, [conn = this->connection]() { conn->Disconnect(); });
		connection.reset();
	}

	context.stop();

	if (thr_context.joinable()) 
	{
		thr_context.join();
	}

	// restart for reusing
	context.restart();
}


bool ft::BaseClient::IsConnected()
{
	if (connection)
	{
		return connection->IsConnected();
	}
	else
	{
		return false;
	}
}
