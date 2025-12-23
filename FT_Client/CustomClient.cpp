#include "CustomClient.h"


CustomClient::~CustomClient()
{
	if (running)
	{
		StopThreadHandleMessages();
	}
}

bool CustomClient::CheckFile(const std::string& file)
{
	if (in.is_open())
	{
		in.close();
	}

	in.open(file, std::ios::binary);
	if (!in.is_open())
	{
		if (onMessage) onMessage("cant open file. path must contains only Eng letters");
		return false;
	}

	// get file name
	size_t pos = file.find_last_of('/\\');
	std::string dir = file.substr(0, pos + 1);
	std::string filename = (pos == std::string::npos) ? file : file.substr(pos + 1);
	// get file size
	in.seekg(0, std::ios::end);
	MaxValue = in.tellg();
	currentValue = 0;
	in.seekg(0);
	in.close();

	connection->current_file.SetFileDir(dir);
	connection->current_file.SetFileName(filename);
	connection->current_file.SetRefClient(connection);
	if (!connection->current_file.OpenFile())
	{
		if (onMessage) onMessage("connection cant open file");
		return false;
	}
	return true;
}

void CustomClient::progress()
{
	uint64_t currentMB = (currentValue) / (1024.0 * 1024.0);
	uint64_t maxMB = (MaxValue) / (1024.0 * 1024.0);

	if (onProgress) onProgress(currentMB, maxMB);
}

bool CustomClient::UploadFile(std::string& file)
{

	if (IsConnected())
	{

		if (CheckFile(file))
		{
			// start uploading file. 
			temprorary_msg.body.clear();
			temprorary_msg.header.msg_type = ft::MessageType::UploadFile;
			temprorary_msg.setString("");
			connection->Send(temprorary_msg);

			// send filesize and wait for answer from server
			if (!connection->current_file.SendFileSize())
			{
				return false;
			}
			
			
			return true;
		}

	}
	return false;
}

void CustomClient::ContinueUploadFile(std::string& id)
{
	if (IsConnected())
	{
		temprorary_msg.body.clear();
		temprorary_msg.header.msg_type = ft::MessageType::ContinueUploadFile;
		temprorary_msg.setString(id);
		connection->Send(temprorary_msg);
	}
}

void CustomClient::DownloadFile(std::string& id)
{
	if (IsConnected())
	{
		temprorary_msg.body.clear();
		temprorary_msg.header.msg_type = ft::MessageType::DownloadFile;
		temprorary_msg.setString(id);
		connection->Send(temprorary_msg);
	}
}

void CustomClient::HandleReceivedMsg()
{
	while (running)
	{
		// wait for messages from server
		get_IncomingMsg().wait();

		if (!running)
		{
			break;
		}

		if (get_IncomingMsg().empty())
		{
			continue;
		}

		// get message
		auto msg = get_IncomingMsg().pop_front().msg;

		// put message to processing queue
		{
			std::lock_guard<std::mutex> lock(m_processingMutex);
			m_processingQueue.push(msg);
		}

		// notify processing thread
		m_processingCV.notify_one();
	}
}

void CustomClient::ProcessMessages()
{
	while (running)
	{

		ft::Message msg;

		// wait for messages from "HandleReceivedMsg" method
		{
			std::unique_lock<std::mutex> lock(m_processingMutex);
			m_processingCV.wait(lock, [this] { return !m_processingQueue.empty() || !running; });

			if (!running && m_processingQueue.empty())
			{
				break;
			}

			// get new message and handle it
			msg = m_processingQueue.front();
			m_processingQueue.pop();
		} 


		// if user clicked Cancel button
		if (IsCancelTranseringFile.load())
		{
			temprorary_msg.body.clear();
			temprorary_msg.header.msg_type = ft::MessageType::CancelTransfer;
			temprorary_msg.setString("");
			connection->Send(temprorary_msg);

			std::queue<ft::Message> empty;
			std::swap(m_processingQueue, empty);

			IsCancelTranseringFile.store(false);

			connection->current_file.Clear();

			if (onMessage) onMessage("File transfering stopped");
			if (onFinished) onFinished();

			continue;
		}


		// handle messages
		if ((msg.header.msg_type == ft::MessageType::FileNotExist))
		{
			if (onMessage) onMessage("File not found or id is incorrect");
			if (onFinished) onFinished();
		}
		else if ((msg.header.msg_type == ft::MessageType::NotEnoughSpace))
		{
			if (onMessage) onMessage("Server storage has not free space");
			if (onFinished) onFinished();
			connection->current_file.Clear();
		}
		else if (msg.header.msg_type == ft::MessageType::FileTooBig)
		{
			uint32_t maxFilesize = 0;
			std::memcpy(&maxFilesize, msg.body.data(), sizeof(maxFilesize));
			if (onMessage) onMessage("File size must be < " + std::to_string(maxFilesize) + " GB");
			if (onFinished) onFinished();
			connection->current_file.Clear();
		}
		else if ((msg.header.msg_type == ft::MessageType::Accept))
		{
			if (!connection->current_file.SendFileName())
			{
				if (onMessage) onMessage("cant send filename");
			}
		}
		else if ((msg.header.msg_type == ft::MessageType::AcceptContinue))
		{
			if (!connection->current_file.SendChunk(true))
			{
				if (onMessage) onMessage("cant send filechunk after acceptContinue");
			}
		}
		else if (msg.header.msg_type == ft::MessageType::GetPartFile)
		{
			uint64_t server_fileCurrentSize = 0;
			std::memcpy(&server_fileCurrentSize, msg.body.data(), sizeof(server_fileCurrentSize));
			connection->current_file.SetInPointer(server_fileCurrentSize);
			
			if (!connection->current_file.SendFileSize())
			{
				if (onMessage) onMessage("cant send fileSize after GetPartFile");
			}
		}
		else if (msg.header.msg_type == ft::MessageType::FileName)
		{
			std::string fname(msg.body.begin(), msg.body.end());

			// try open file
			// if exist -> continue downloading
			// if not -> download whole file

			connection->current_file.Init(pathForDownloads, fname);

			if (!connection->current_file.StartNewFile())
			{
				if (onMessage) onMessage("cant create file for downloading");
			}

			connection->current_file.SetRefClient(connection);
			MaxValue = connection->current_file.CalculateFileSizeOUT();
			if (MaxValue > 0)
			{
				// continue downloading after disconnect
				// move pointer and send current file size to server
				connection->current_file.SetReceivedSize(MaxValue);
				connection->current_file.SetOutPointer(MaxValue);

				temprorary_msg.body.clear();
				temprorary_msg.header.msg_type = ft::MessageType::GetPartFile;
				temprorary_msg.setSimpleData(MaxValue);
				connection->Send(temprorary_msg);
			}
			else
			{
				// download whole file
				temprorary_msg.body.clear();
				temprorary_msg.header.msg_type = ft::MessageType::GetWholeFile;
				temprorary_msg.setString("");
				connection->Send(temprorary_msg);
			}

		}
		else if (msg.header.msg_type == ft::MessageType::FileSize)
		{
			uint64_t fsize;
			std::memcpy(&fsize, msg.body.data(), sizeof(fsize));
			if (fsize == MaxValue)
			{
				// file already downloaded. just finish the work
				connection->current_file.Clear();
				temprorary_msg.body.clear();
				temprorary_msg.header.msg_type = ft::MessageType::FinishedDownload;
				temprorary_msg.setString("");
				connection->Send(temprorary_msg);
			}
			else
			{
				MaxValue = fsize;
				connection->current_file.SetFileSize(MaxValue);
				currentValue = connection->current_file.GetReceivedsize();
				progress();
			}
		}
		else if (msg.header.msg_type == ft::MessageType::FileChunk)
		{

			if (!connection->current_file.WriteChunk(msg.body.data(), msg.header.size))
			{
				if (onMessage) onMessage("cant write chunk");
			}

			if (connection->current_file.IsFileReceived)
			{
				// downloading done
				if (onMessage) onMessage("Calculating hash file...");

				if (!connection->current_file.StartCheckHash())
				{
					if (onMessage) onMessage("error while calculating hash");
				}
				if (onMessage) onMessage("Calculating hash done");

			}
			else
			{
				uint32_t tempSize = connection->current_file.GetReceivedsize();
				if (tempSize)
				{
					currentValue = tempSize;
					progress();
				}

				temprorary_msg.body.clear();
				temprorary_msg.setString("");
				temprorary_msg.header.msg_type = ft::MessageType::ReceivedChunk;
				connection->Send(temprorary_msg);

			}

		}
		else if (msg.header.msg_type == ft::MessageType::FileID)
		{
			std::string f_id(msg.body.begin(), msg.body.end());

			if (onMessage) onMessage("ID for your file: " + f_id);

			// start sending file
			if (!connection->current_file.SendChunk(true))
			{
				if (onMessage) onMessage("cant send first chunk");
			}
		}
		else if (msg.header.msg_type == ft::MessageType::ReceivedChunk)
		{
			std::memcpy(&currentValue, msg.body.data(), sizeof(currentValue));
			progress();

			// next chunk
			if (!connection->current_file.SendChunk())
			{
				if (onMessage) onMessage("cant send filechunk");
			}
		}
		else if (msg.header.msg_type == ft::MessageType::CalculatingHash)
		{
			if (onMessage) onMessage("Calculating hash file...");
			uint64_t hash_client = ft::filehash::CalculateSimpleFileHash(connection->current_file.GetDir());
			connection->current_file.SetFileHash(hash_client);

			if (onMessage) onMessage("calculating hash done");
		}
		else if (msg.header.msg_type == ft::MessageType::HashFile)
		{
			uint64_t hash_server;
			std::memcpy(&hash_server, msg.body.data(), sizeof(hash_server));

			if (connection->current_file.GetHash() == hash_server)
			{
				temprorary_msg.body.clear();
				temprorary_msg.setString("");
				temprorary_msg.header.msg_type = ft::MessageType::HashSimilar;
				connection->Send(temprorary_msg);
			}
			else
			{
				// hashes not similar
				temprorary_msg.body.clear();
				temprorary_msg.setString("");
				temprorary_msg.header.msg_type = ft::MessageType::HashWrong;
				connection->Send(temprorary_msg);
				
				// finish work
				goto HashesNOTSimilarOnClient;
			}
		}
		else if (msg.header.msg_type == ft::MessageType::HashWrong)
		{
			HashesNOTSimilarOnClient:

			if (onMessage) onMessage("Hash File not equal to hash on server. Try Again");
			if (onFinished) onFinished();
			connection->current_file.Clear();
		}
		else if (msg.header.msg_type == ft::MessageType::HashCalculatingError)
		{
			if (onMessage) onMessage("Server cant calculate hash");
			if (onFinished) onFinished();
			connection->current_file.Clear();
		}
		else if (msg.header.msg_type == ft::MessageType::WrongFile)
		{
			if (onMessage) onMessage("You're trying to upload other file with this ID. Try again");
			if (onFinished) onFinished();
			connection->current_file.Clear();
		}
		else if (msg.header.msg_type == ft::MessageType::UnknownError)
		{
			if (onMessage) onMessage("Unknown error. Please, try again");
			if (onFinished) onFinished();
			connection->current_file.Clear();
		}
		else if (msg.header.msg_type == ft::MessageType::FinishedUpload)
		{
			if (onMessage) onMessage("UPLOADING DONE");
			if (onFinished) onFinished();

			currentValue = 0;
		}
		else if (msg.header.msg_type == ft::MessageType::FinishedDownload)
		{
			if (onMessage) onMessage("DOWNOADING DONE");
			if (onFinished) onFinished();

			currentValue = 0;
			connection->current_file.Clear();
		}

	}

}

void CustomClient::StartThreadHandleMessages()
{
	if (running)
	{
		return;
	}

	running = true;

	thr_reader = std::thread([this]() { HandleReceivedMsg(); });
	thr_processor = std::thread([this]() { ProcessMessages(); });
}

void CustomClient::StopThreadHandleMessages()
{
	if (!running)
	{
		return;
	}

	running = false;

	get_IncomingMsg().wakeUpCV();

	m_processingCV.notify_one();


	if (thr_reader.joinable())
	{
		thr_reader.join();
	}
	if (thr_processor.joinable())
	{
		thr_processor.join();
	}

	// clear queue
	std::queue<ft::Message> empty;
	std::swap(m_processingQueue, empty);
}

void CustomClient::CancelTranseringFile()
{
	if (!running) return;

	IsCancelTranseringFile.store(true); // stop upload/download
	ft::filehash::IsCancelCalculatingHash.store(true); // stop calculating hash
}