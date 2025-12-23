#include "FileTransferState.h"
#include "BaseServer.h"


ft::FileTransferState::~FileTransferState()
{
	Clear();
}

void ft::FileTransferState::Init(const std::string& dir, const std::string& fname, uint64_t fsize, const std::string& fid, ft::BaseServer* ref_server, std::shared_ptr<ft::Connection> ref_client)
{
	this->dir = dir;
	this->filename = fname;
	this->fileID = fid;
	this->filesize = fsize;
	this->ref_server = ref_server;
	this->ref_client = ref_client;
}

void ft::FileTransferState::SetFileDir(const std::string& dir) { this->dir = dir; }
void ft::FileTransferState::SetFileName(const std::string& fname) { this->filename = fname; }
void ft::FileTransferState::SetFileID(const std::string& fid) { this->fileID = fid; }
void ft::FileTransferState::SetFileHash(const uint64_t fhash) { this->fileHash = fhash; }
void ft::FileTransferState::SetRefServer(ft::BaseServer* ref_server) { this->ref_server = ref_server; }
void ft::FileTransferState::SetRefClient(std::shared_ptr<ft::Connection> ref_client) { this->ref_client = ref_client; }
void ft::FileTransferState::SetBufferSize(uint16_t buffer_size) { this->buffer_size = buffer_size; }
void ft::FileTransferState::SetFileSize(uint64_t size) { this->filesize = size; }
void ft::FileTransferState::SetReceivedSize(uint64_t size) { this->received_size = size; }

bool ft::FileTransferState::SetInPointer(uint64_t pos)
{
	if (in.is_open())
	{
		in.seekg(pos);
		return true;
	}
	return false;
}
bool ft::FileTransferState::SetOutPointer(uint64_t pos)
{
	if (out.is_open())
	{
		out.seekp(pos);
		return true;
	}
	return false;
}

void ft::FileTransferState::CloseFile()
{
	if (in.is_open())
	{
		in.close();
	}
	if (out.is_open())
	{
		out.close();
	}
}
void ft::FileTransferState::Clear()
{
	dir.clear();
	filename.clear();
	fileID.clear();
	filesize = 0;
	received_size = 0;
	fileHash = 0;
	ref_server = nullptr;
	ref_client = nullptr;
	in.close();
	out.close();
	isContinueUploading = false;
	IsFileReceived = false;
}

bool ft::FileTransferState::SendFromServerToClient()
{
	if (ref_server && ref_client)
	{
		ref_client->Send(temproraryMsgOut);
		return true;
	}
	else
	{
		return false;
	}
}
bool ft::FileTransferState::SendFromClientToServer()
{
	if (ref_client)
	{
		ref_client->Send(temproraryMsgOut);
		return true;
	}
	else
	{
		return false;
	}
}

bool ft::FileTransferState::StartNewFile()
{
	IsFileReceived = false;

	if (out.is_open())
	{
		out.close();
	}

	received_size = 0;

	dir += filename;
	out.open(dir, std::ios::binary | std::ios::app);
	if (!out.is_open())
	{
		return false;
	}
	return true;
}

bool ft::FileTransferState::OpenFile()
{
	IsFileReceived = false;

	if (in.is_open())
	{
		in.close();
	}

	if (dir != filename)
	{
		dir += filename;
	}
	in.open(dir, std::ios::binary);

	if (!in.is_open())
	{
		return false;
	}

	filesize = CalculateFileSizeIN();
	return true;
}

bool ft::FileTransferState::SendFileSize()
{
	temproraryMsgOut.body.clear();
	temproraryMsgOut.header.msg_type = ft::MessageType::FileSize;
	temproraryMsgOut.setSimpleData(filesize);
	if (ref_server) // if server
	{
		if (!SendFromServerToClient())
		{
			return false;
		}

	}
	else // if client
	{
		if (!SendFromClientToServer())
		{
			return false;
		}
	}
	return true;
}

bool ft::FileTransferState::SendFileName()
{

	temproraryMsgOut.body.clear();
	temproraryMsgOut.header.msg_type = ft::MessageType::FileName;
	temproraryMsgOut.setString(filename);
	if (ref_server) // if server
	{
		if (!SendFromServerToClient())
		{
			return false;
		}
	}
	else // if client
	{
		if (!SendFromClientToServer())
		{
			return false;
		}
	}

	return true;
	
}

bool ft::FileTransferState::SendChunk(bool isFirstChunk)
{
	if (!in.is_open())
	{
		return false;
	}

	if (in)
	{

		if (isFirstChunk)
		{
			temproraryMsgOut.body.clear();
			temproraryMsgOut.header.msg_type = ft::MessageType::FileChunk;
			temproraryMsgOut.body.resize(buffer_size);
		}


		in.read(temproraryMsgOut.body.data(), temproraryMsgOut.body.size());
		std::streamsize count = in.gcount();
		if (count > 0)
		{
			temproraryMsgOut.body.resize(static_cast<size_t>(count));
			temproraryMsgOut.header.size = static_cast<uint64_t>(count);
			if (ref_server) // if server
			{
				if (!SendFromServerToClient())
				{
					Clear();
					return false;
				}
			}
			else // if client
			{
				if (!SendFromClientToServer())
				{
					Clear();
					return false;
				}
			}
		}

	}
	else
	{
		in.close();
	}

	return true;
}

bool ft::FileTransferState::WriteChunk(const char* data, size_t len)
{
	if (!out.is_open())
	{
		return false;
	}

	if (data)
	{
		out.write(data, len);
		received_size += len;
	}

	// when finished
	if (received_size >= filesize)
	{
		IsFileReceived = true;
		isContinueUploading = false;
	}

	return true;

}

bool ft::FileTransferState::StartCheckHash()
{
	out.close();

	// calculate hash

	temproraryMsgOut.body.clear();
	temproraryMsgOut.header.msg_type = ft::MessageType::CalculatingHash;
	temproraryMsgOut.setString("");
	if (ref_server) // if server
	{
		if (!SendFromServerToClient())
		{
			return false;
		}
	}
	else // if client
	{
		if (!SendFromClientToServer())
		{
			return false;
		}
	}

	temproraryMsgOut.body.clear();
	fileHash = ft::filehash::CalculateSimpleFileHash(dir);


	if (fileHash)
	{
		temproraryMsgOut.header.msg_type = ft::MessageType::HashFile;
		temproraryMsgOut.setSimpleData(fileHash);
	}
	else
	{
		temproraryMsgOut.header.msg_type = ft::MessageType::HashCalculatingError;
		temproraryMsgOut.setString("");
	}

	// send hash
	if (ref_server) // if server
	{
		if (!SendFromServerToClient())
		{
			return false;
		}
	}
	else // if client
	{
		if (!SendFromClientToServer())
		{
			return false;
		}
	}

	return true;
}

uint64_t ft::FileTransferState::CalculateFileSizeIN()
{
	if (in.is_open())
	{
		auto cur_pos = in.tellg();
		in.seekg(0, std::ios::end);
		uint64_t tempsize = in.tellg();
		in.seekg(cur_pos);
		return tempsize;
	}

	return 0;
}
uint64_t ft::FileTransferState::CalculateFileSizeOUT()
{
	if (out.is_open())
	{
		auto cur_pos = out.tellp();
		out.seekp(0, std::ios::end);
		uint64_t tempsize = out.tellp();
		out.seekp(cur_pos);
		return tempsize;
	}

	return 0;
}