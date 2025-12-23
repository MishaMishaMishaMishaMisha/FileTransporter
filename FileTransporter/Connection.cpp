#include "Connection.h"

#include "BaseServer.h"


ft::Connection::Connection(OwnerType owner, asio::io_context& context, asio::ip::tcp::socket socket, ft::TSQueue<ft::OwnedMessage>& q_in)
	: context(context), socket(std::move(socket)), queue_msg_in(q_in)
{
	this->owner = owner;

	// prepare for validation. server generate data and calculate data_check
	if (owner == OwnerType::Server)
	{
		// data is current time in miliseconds
		val_data_out = uint64_t(std::chrono::system_clock::now().time_since_epoch().count());
		val_data_check = encryptValidatingData(val_data_out);
	}
	else
	{
		val_data_in = 0;
		val_data_out = 0;
	}
}

ft::Connection::~Connection()
{

}


void ft::Connection::ConnectToClient(ft::BaseServer* server, uint32_t uid)
{
	if (owner == OwnerType::Server)
	{
		if (socket.is_open())
		{
			id = uid;

			// firstly, server send data to client for validation
			// and wait for answer
			WriteValidation();
			ReadValidation(server);
		}
	}
}

void ft::Connection::ConnectToServer(const asio::ip::tcp::resolver::results_type& serverInfo, std::function<void(bool success)> on_complete)
{
	if (owner == OwnerType::Client)
	{
		asio::async_connect(socket, serverInfo, 
			[this, on_complete](std::error_code ec, asio::ip::tcp::endpoint endpoints)
			{
				if (!ec)
				{
					ReadValidation();
					on_complete(true);
				}
				else
				{
					socket.close();
					on_complete(false);
				}
			});
	}
}

void ft::Connection::Disconnect()
{
	if (IsConnected())
	{
		asio::post(context, [this]() 
			{
			std::error_code ec;
			socket.shutdown(asio::ip::tcp::socket::shutdown_both, ec);
			socket.close(ec);
			});
	}
}

bool ft::Connection::IsConnected() const
{
	if (socket.is_open())
	{
		return true;
	}

	return false;
}


void ft::Connection::Send(const ft::Message& msg)
{
	asio::post(context,
		[this, msg]()
		{
			bool IsWritingMessage = !queue_msg_out.empty();
			queue_msg_out.push_back(msg);
			if (!IsWritingMessage)
			{
				WriteHeader();
			}
		});

}


// async methods for reading,writing
void ft::Connection::ReadHeader()
{
	auto self = this->shared_from_this();
	asio::async_read(socket, asio::buffer(&temproraryMsgIn.header, sizeof(ft::MessageHeader)),
		[this, self](std::error_code ec, size_t length)
		{
			if (!ec)
			{
				if (temproraryMsgIn.header.size > 0)
				{
					temproraryMsgIn.body.resize(temproraryMsgIn.header.size);
					ReadBody();
				}
				else
				{
					// call ReadHeader() again
					AddToIncomingMsgQueue();
				}
			}
			else
			{
				// notice client about connection lost
				if (owner == OwnerType::Client && IsConnected())
				{
					if (onConnectionLost) onConnectionLost();
				}
				socket.close();
			}
		});
}
void ft::Connection::ReadBody()
{
	auto self = this->shared_from_this();
	asio::async_read(socket, asio::buffer(temproraryMsgIn.body.data(), temproraryMsgIn.body.size()),
		[this, self](std::error_code ec, size_t length)
		{
			if (!ec)
			{
				AddToIncomingMsgQueue();
			}
			else
			{
				// notice client about connection lost
				if (owner == OwnerType::Client && IsConnected())
				{
					if (onConnectionLost) onConnectionLost();
				}
				socket.close();
			}
		});
}
void ft::Connection::WriteHeader()
{
	auto self = this->shared_from_this();
	asio::async_write(socket, asio::buffer(&queue_msg_out.front().header, sizeof(ft::MessageHeader)),
		[this, self](std::error_code ec, size_t length)
		{
			if (!ec)
			{
				if (queue_msg_out.front().body.size() > 0)
				{
					WriteBody();
				}
				else
				{
					queue_msg_out.pop_front();
					if (!queue_msg_out.empty())
					{
						WriteHeader();
					}
				}
			}
			else
			{
				// notice client about connection lost
				if (owner == OwnerType::Client && IsConnected())
				{
					if (onConnectionLost) onConnectionLost();
				}
				socket.close();
			}
		});
}
void ft::Connection::WriteBody()
{
	auto self = this->shared_from_this();
	asio::async_write(socket, asio::buffer(queue_msg_out.front().body.data(), queue_msg_out.front().body.size()),
		[this, self](std::error_code ec, size_t length)
		{
			if (!ec)
			{
				queue_msg_out.pop_front();
				if (!queue_msg_out.empty())
				{
					WriteHeader();
				}
			}
			else
			{
				// notice client about connection lost
				if (owner == OwnerType::Client && IsConnected())
				{
					if (onConnectionLost) onConnectionLost();
				}
				socket.close();
			}
		});
}

// add temproraryMessage to Queue and call ReadHeader again
void ft::Connection::AddToIncomingMsgQueue()
{
	if (owner == OwnerType::Server)
	{
		
		// request for download file from server push to beginnning of queue
		if (temproraryMsgIn.header.msg_type == ft::MessageType::DownloadFile)
		{
			queue_msg_in.push_front({ this->shared_from_this(), temproraryMsgIn });
		}
		else
		{
			queue_msg_in.push_back({ this->shared_from_this(), temproraryMsgIn });
		}
	}
	else
	{
		queue_msg_in.push_back({ nullptr, temproraryMsgIn });
	}

	// start new reading task
	ReadHeader();
}



// Validating
// encrypt data 
uint64_t ft::Connection::encryptValidatingData(uint64_t inputData)
{
	uint64_t outData = inputData ^ 0xABCDEF1234567890;
	outData = ((~outData & 0xFF00FF00FF00FF00) >> 8) | ((outData & 0x00FF00FF00FF00FF) << 8);
	return outData ^ 0x1234FACECAFEBEEF;
}

void ft::Connection::WriteValidation()
{
	auto self = this->shared_from_this();
	asio::async_write(socket, asio::buffer(&val_data_out, sizeof(uint64_t)),
		[this, self](std::error_code ec, size_t length)
		{
			if (!ec)
			{
				// if this is client
				// after sending validation data to server
				// start wait for messages 
				if (owner == OwnerType::Client)
				{
					ReadHeader();
				}
			}
			else
			{
				// notice client about connection lost
				if (owner == OwnerType::Client && IsConnected())
				{
					if (onConnectionLost) onConnectionLost();
				}
				socket.close();
			}
		});
}

void ft::Connection::ReadValidation(ft::BaseServer* server)
{
	auto self = this->shared_from_this();
	asio::async_read(socket, asio::buffer(&val_data_in, sizeof(uint64_t)),
		[this, server, self](std::error_code ec, size_t length)
		{
			if (!ec)
			{
				if (owner == OwnerType::Server)
				{
					// if this is server
					// check if data_fromClient == data_check
					// if true - start waiting for messages
					if (val_data_in == val_data_check)
					{
						server->OnClientValidated(this->shared_from_this());
						ReadHeader();
					}
					else
					{
						socket.close();
					}
				}
				else
				{
					// if this is client
					// calculate output data and send to server
					val_data_out = encryptValidatingData(val_data_in);
					WriteValidation();
				}
			}
			else
			{
				// notice client about connection lost
				if (owner == OwnerType::Client && IsConnected())
				{
					if (onConnectionLost) onConnectionLost();
				}
				socket.close();
			}
		});
}