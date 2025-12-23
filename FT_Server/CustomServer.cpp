#include "CustomServer.h"


CustomServer::CustomServer(const uint16_t port) : ft::BaseServer(port), thr_pool(num_threads)
{

	// disable automatic check connections. server check it by timers
	SetCheckConncectionEnable(false);

}

CustomServer::~CustomServer()
{

}


void CustomServer::setStoragePath(std::string& path)
{
	if (path.size() == 0)
	{
		return;
	}
	if (path[path.size() - 1] != '\\')
	{
		path += "\\";
	}

	if (!ft::fm::IsPathValid(path))
	{
		return;
	}

	storagePath = path;

	isStorageCreated = false;
	
	CreateStorage();

}

void CustomServer::setStorageSpace(int gb)
{ 
	storageSpaceGB = gb;
	storageSpace = static_cast<uint64_t>(storageSpaceGB) * 1024 * 1024 * 1024; 
}

void CustomServer::setMaxUserFileSize(int gb)
{
	max_user_filesize_GB = gb;
	max_user_filesize = static_cast<uint64_t>(max_user_filesize_GB) * 1024 * 1024 * 1024;
}


bool CustomServer::OnClientConnect(std::shared_ptr<ft::Connection> client)
{
	if (onMessage) onMessage("New client try to connect");
	return true; 
}

void CustomServer::OnClientValidated(std::shared_ptr<ft::Connection> client)
{
	std::string s = "Client [" + std::to_string(client->GetID()) + "] Connected";
	if (onMessage) onMessage(s);

	connectedUsers += 1;
	if (onUsersCountChanged) onUsersCountChanged(connectedUsers);
}

void CustomServer::OnClientDisconnect(std::shared_ptr<ft::Connection> client)
{
	if (!client)
	{
		return;
	}

	std::string s = "Client [" + std::to_string(client->GetID()) + "] Disconnected";
	if (onMessage) onMessage(s);
	client->current_file.Clear();

	if (connectedUsers > 0)
	{
		connectedUsers -= 1;
	}
	if (onUsersCountChanged) onUsersCountChanged(connectedUsers);
}

void CustomServer::OnServerStarted()
{

	thr_pool.start();

	get_IncomingMsg().clear();

	connectedUsers = 0;
	if (onUsersCountChanged) onUsersCountChanged(connectedUsers);


	if (onMessage) onMessage("Server started");

}

void CustomServer::OnServerStopped()
{

	thr_pool.stop();

	if (onMessage) onMessage("Server stopped");

	connectedUsers = 0;
	if (onUsersCountChanged) onUsersCountChanged(connectedUsers);

}

bool CustomServer::moveDirFromTempToStorage(const std::string& name)
{
	return ft::fm::moveDir(tempStoragePath + name, storagePathFull + name);
}

void CustomServer::AddFile(std::string& fileID, uint64_t fileSize, uint64_t hash)
{
	mtx.lock();
	auto now = std::time(nullptr);
	files[fileID] = std::make_pair(fileSize, now);
	hash_files[fileID] = hash;

	currentFiles = files.size();
	if (onFilesChanged) onFilesChanged(currentFiles);

	// add to file
	fileStorage.addEntry(fileID, fileSize, now, hash);

	mtx.unlock();
}
void CustomServer::AddTempFile(std::string& fileID, uint64_t fileSize)
{
	mtx_temp.lock();
	auto now = std::time(nullptr);
	temp_files[fileID] = std::make_pair(fileSize, now);

	currentTempFiles = temp_files.size();
	if (onTempFilesChanged) onTempFilesChanged(currentTempFiles);

	// add to file
	fileStorage.addTempEntry(fileID, fileSize, now);

	mtx_temp.unlock();
}
void CustomServer::RemoveFile(std::string& fileID)
{
	mtx.lock();
	files.erase(fileID);

	currentFiles = files.size();
	if (onFilesChanged) onFilesChanged(currentFiles);

	// remove from file
	fileStorage.removeEntries({ fileID });

	mtx.unlock();
}
void CustomServer::RemoveTempFile(std::string& fileID)
{
	mtx.lock();
	temp_files.erase(fileID);

	currentTempFiles = temp_files.size();
	if (onTempFilesChanged) onTempFilesChanged(currentTempFiles);

	// remove from file
	fileStorage.removeTempEntries({ fileID });

	mtx.unlock();
}

std::string CustomServer::generateID()
{
	// id = current time + random number
	auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
	static thread_local std::mt19937_64 rng(std::random_device{}());
	std::uniform_int_distribution<unsigned long long> dist;
	unsigned long long randomPart = dist(rng);
	std::stringstream ss;
	ss << std::hex << now << "-" << randomPart;
	return ss.str();
}

void CustomServer::CleanStorage()
{
	using namespace std::chrono_literals;
	if (IsRunning)
	{

		if (onMessage) onMessage("Start cleaning old files");

		std::unordered_set<std::string> IDs;

		auto now = std::time(nullptr);
		for (auto it = files.begin(); it != files.end(); )
		{
			if (now - it->second.second > max_time_file_existing)
			{

				std::string cur_id = it->first;
				
				// delete folder with file
				if (!ft::fm::removeDir(storagePathFull + cur_id))
				{
					if (onMessage) onMessage("error while deleting folder with file");

				}

				currentSpace -= it->second.first;


				IDs.insert(cur_id);

				// remove from dict
				hash_files.erase(cur_id);
				it = files.erase(it);

				if (onMessage) onMessage("file " + cur_id + " deleted");

			}
			else
			{
				++it;
			}
		}

		// remove from txt file
		fileStorage.removeEntries(IDs);

		currentFiles = files.size();
		if (onFilesChanged) onFilesChanged(currentFiles);
		if (onCurrentSpaceChanged) onCurrentSpaceChanged(currentSpace, storageSpace);

		if (onMessage) onMessage("Cleaning finished");
	}
}

void CustomServer::CleanTemp()
{
	using namespace std::chrono_literals;
	if (IsRunning)
	{

		if (onMessage) onMessage("Start cleaning temp files");

		std::unordered_set<std::string> IDs;

		auto now = std::time(nullptr);
		for (auto it = temp_files.begin(); it != temp_files.end(); )
		{
			if (now - it->second.second > max_time_tempFile_existing)
			{

				std::string cur_id = it->first;

				// delete folder with file
				if (!ft::fm::removeDir(tempStoragePath + cur_id))
				{
					if (onMessage) onMessage("error while deleting folder with temp file");

				}
				reservedSpace -= it->second.first;

				IDs.insert(cur_id);

				// remove from dict 
				it = temp_files.erase(it);

				if (onMessage) onMessage("temp file " + cur_id + " deleted");

			}
			else
			{
				++it;
			}
		}

		// remove from txt file
		fileStorage.removeTempEntries(IDs);

		currentTempFiles = temp_files.size();
		if (onTempFilesChanged) onTempFilesChanged(currentTempFiles);
		if (onReservedSpaceChanged) onReservedSpaceChanged(reservedSpace);

		if (onMessage) onMessage("Cleaning temp finished");
	}
}

void CustomServer::CleanFullStorage()
{
	if (!ft::fm::removeDir(storagePathFull))
	{
		if (onMessage) onMessage("Error while clearing storage");
	}

	files.clear();
	temp_files.clear();
	hash_files.clear();

	isStorageCreated = false;
	ApplySettings();

	if (onMessage) onMessage("Storage cleared");
}


// handle incoming messages
void CustomServer::OnMessage(std::shared_ptr<ft::Connection> client, const ft::Message& msg)
{
	if (msg.header.msg_type == ft::MessageType::UploadFile)
	{
		// after this, client send filesize
		std::string text = "Client[" + std::to_string(client->GetID()) + "]: try to upload file";
		if (onMessage) onMessage(text);
	}
	else if (msg.header.msg_type == ft::MessageType::ContinueUploadFile)
	{
		// read file id and try to find file in temp folder
		std::string f_id(msg.body.begin(), msg.body.end());

		// delegate task to threads pool
		auto client_ptr = client;
		thr_pool.enqueue([this, client_ptr, f_id]()
			{

				auto now = std::time(nullptr);
				if (isCleaningEnable && (now - temp_files[f_id].second > max_time_tempFile_existing))
				{
					// client try to continue upload old file
					std::string text = "Client[" + std::to_string(client_ptr->GetID()) + "]: try to continue upload old file. Reject";
					if (onMessage) onMessage(text);

					ft::Message msg;
					msg.body.clear();
					msg.header.msg_type = ft::MessageType::FileNotExist;
					msg.setString("");
					client_ptr->Send(msg);

					return;
				}


				std::string f_name;
				std::string id = f_id;
				if (ft::fm::FindFileByDirName(tempStoragePath, id, f_name))
				{

					std::string text = "Client[" + std::to_string(client_ptr->GetID()) + "]: try to continue upload file";
					if (onMessage) onMessage(text);

					client_ptr->current_file.SetFileName(f_name);
					client_ptr->current_file.SetFileDir(tempStoragePath + f_id + "\\");
					client_ptr->current_file.SetRefServer(this);
					client_ptr->current_file.SetRefClient(client_ptr);
					client_ptr->current_file.SetFileID(f_id);

					if (!client_ptr->current_file.StartNewFile())
					{
						if (onMessage) onMessage("cant create file for ContinueUploaing");
					}
					
					// move pointer in file
					uint64_t current_received_size = client_ptr->current_file.CalculateFileSizeOUT();
					client_ptr->current_file.SetReceivedSize(current_received_size);
					client_ptr->current_file.SetOutPointer(current_received_size);

					client_ptr->current_file.isContinueUploading = true;

					// send request for uploading other part of file
					ft::Message msg;
					msg.body.clear();
					msg.header.msg_type = ft::MessageType::GetPartFile;
					msg.setSimpleData(current_received_size);
					client_ptr->Send(msg);

				}
				else
				{
					std::string text = "Client[" + std::to_string(client_ptr->GetID()) + "]: try to continue upload wrong file. Reject";
					if (onMessage) onMessage(text);

					ft::Message msg;
					msg.body.clear();
					msg.header.msg_type = ft::MessageType::FileNotExist;
					msg.setString("");
					client_ptr->Send(msg);
				}

			});

	}
	else if (msg.header.msg_type == ft::MessageType::DownloadFile)
	{

		// find file by id
		std::string f_id(msg.body.begin(), msg.body.end());

		// delegate task to threads pool
		auto client_ptr = client;
		thr_pool.enqueue([this, client_ptr, f_id]()
			{

				auto now = std::time(nullptr);
				if (isCleaningEnable && now - files[f_id].second > max_time_file_existing)
				{
					// client try to download old file
					std::string text = "Client[" + std::to_string(client_ptr->GetID()) + "]: try to download old file. Reject";
					if (onMessage) onMessage(text);

					ft::Message msg;
					msg.body.clear();
					msg.header.msg_type = ft::MessageType::FileNotExist;
					msg.setString("");
					client_ptr->Send(msg);

					return;
				}


				std::string f_name;
				std::string id = f_id;
				if (ft::fm::FindFileByDirName(storagePathFull, id, f_name))
				{
					std::string text = "Client[" + std::to_string(client_ptr->GetID()) + "]: try to download file. Accept";
					if (onMessage) onMessage(text);

					client_ptr->current_file.SetFileName(f_name);
					client_ptr->current_file.SetFileDir(storagePathFull + f_id + "\\");
					if (!client_ptr->current_file.OpenFile())
					{
						std::string text = "Client[" + std::to_string(client_ptr->GetID()) + "]: Error while opening file for downloading";
						if (onMessage) onMessage(text);

						ft::Message msg;
						msg.body.clear();
						msg.setString("");
						msg.header.msg_type = ft::MessageType::UnknownError;
						client_ptr->Send(msg);

						return;
					}
					client_ptr->current_file.SetFileID(f_id);
					client_ptr->current_file.SetRefServer(this);
					client_ptr->current_file.SetRefClient(client_ptr);

					// send filename
					ft::Message msg;
					msg.body.clear();
					msg.header.msg_type = ft::MessageType::FileName;
					msg.setString(client_ptr->current_file.GetFileName());
					client_ptr->Send(msg);

					// wait for answer: getWholeFile or getPartFile

				}
				else
				{
					std::string text = "Client[" + std::to_string(client_ptr->GetID()) + "]: try to download non-existing file. Reject";
					if (onMessage) onMessage(text);

					ft::Message msg;
					msg.body.clear();
					msg.header.msg_type = ft::MessageType::FileNotExist;
					msg.setString("");
					client_ptr->Send(msg);
				}

			});
	}
	else if (msg.header.msg_type == ft::MessageType::CancelTransfer)
	{
		std::string text = "Client[" + std::to_string(client->GetID()) + "]: Cancel transfering file";
		if (onMessage) onMessage(text);
		client->current_file.Clear();
	}
	else if (msg.header.msg_type == ft::MessageType::GetWholeFile)
	{
		// delegate task to threads pool
		auto client_ptr = client;
		thr_pool.enqueue([this, client_ptr]()
			{

				if (!client_ptr->current_file.SendFileSize())
				{
					if (onMessage) onMessage("cant send filesize after getWholeFile");
				}
				if (!client_ptr->current_file.SendChunk(true))
				{
					if (onMessage) onMessage("cant send first fileChunk after getWholeFile");
				}

			});
	}
	else if (msg.header.msg_type == ft::MessageType::GetPartFile)
	{

		uint64_t client_fileCurrentSize = 0;
		std::memcpy(&client_fileCurrentSize, msg.body.data(), sizeof(client_fileCurrentSize));

		// delegate task to threads pool
		auto client_ptr = client;
		thr_pool.enqueue([this, client_ptr, client_fileCurrentSize]()
			{

				client_ptr->current_file.SetInPointer(client_fileCurrentSize);
				
				if (!client_ptr->current_file.SendFileSize())
				{
					if (onMessage) onMessage("cant send filesize after getPartFile");
				}
				if (!client_ptr->current_file.SendChunk(true))
				{
					if (onMessage) onMessage("cant send first fileChunk after getPartFile");
				}

			});
	}
	else if (msg.header.msg_type == ft::MessageType::FileName)
	{

		std::string fname(msg.body.begin(), msg.body.end());

		// delegate task to threads pool
		auto client_ptr = client;
		thr_pool.enqueue([this, client_ptr, fname]()
			{

				std::string id = generateID();
				ft::fm::createDir(tempStoragePath + id);
				client_ptr->current_file.Init(tempStoragePath + id + "\\", fname, client_ptr->current_file.GetFileSieze(), id, this, client_ptr);
				
				if (!client_ptr->current_file.StartNewFile())
				{
					if (onMessage) onMessage("cant create file after FileName");
				}

				AddTempFile(id, client_ptr->current_file.GetFileSieze());

				// send id
				ft::Message msg;
				msg.body.clear();
				msg.header.msg_type = ft::MessageType::FileID;
				msg.setString(id);
				client_ptr->Send(msg);

			});

	}
	else if (msg.header.msg_type == ft::MessageType::FileSize)
	{

		uint64_t fsize = 0;
		std::memcpy(&fsize, msg.body.data(), sizeof(fsize));

		temprorary_msg.body.clear();
		temprorary_msg.setString("");

		// delegate task to threads pool
		auto client_ptr = client;
		thr_pool.enqueue([this, client_ptr, fsize]()
			{

				if (fsize > max_user_filesize)
				{
					std::string text = "Client[" + std::to_string(client_ptr->GetID()) + "]: file is too big. Reject";
					if (onMessage) onMessage(text);

					ft::Message msg;
					msg.header.msg_type = ft::MessageType::FileTooBig;
					msg.setSimpleData(max_user_filesize_GB);
					client_ptr->Send(msg);
					return;
				}

				// if continue uploading, compare fize size and reservedSpace
				// if new file - check free space and send accept or not enough space
				if (client_ptr->current_file.isContinueUploading)
				{
					uint64_t reservedSpaceForThisFile = temp_files[client_ptr->current_file.GetFileID()].first;
					if (fsize != reservedSpaceForThisFile) // other file. decline uploading
					{
						std::string text = "Client[" + std::to_string(client_ptr->GetID()) + "]: try to continue upload wrong file with this ID. Reject";
						if (onMessage) onMessage(text);

						ft::Message msg;
						msg.body.clear();
						msg.header.msg_type = ft::MessageType::WrongFile;
						msg.setString("");
						client_ptr->Send(msg);

						client_ptr->current_file.Clear();
					}
					else if (fsize == client_ptr->current_file.CalculateFileSizeOUT())
					{
						// finish writing file
						client_ptr->current_file.SetFileSize(fsize);
						
						if (!client_ptr->current_file.WriteChunk(nullptr, 0))
						{
							if (onMessage) onMessage("cant write last chunk after continueUploading");
						}
					}
					else
					{
						std::string text = "Client[" + std::to_string(client_ptr->GetID()) + "]: Accept to continue upload file";
						if (onMessage) onMessage(text);

						ft::Message msg;
						msg.body.clear();
						msg.header.msg_type = ft::MessageType::AcceptContinue;
						msg.setString("");
						client_ptr->Send(msg);

						client_ptr->current_file.SetFileSize(fsize);
					}

				}
				else if (currentSpace + fsize + reservedSpace >= storageSpace)
				{
					std::string text = "Client[" + std::to_string(client_ptr->GetID()) + "]: Reject to upload file. No more free space in storage";
					if (onMessage) onMessage(text);

					ft::Message msg;
					msg.body.clear();
					msg.header.msg_type = ft::MessageType::NotEnoughSpace;
					msg.setString("");
					client_ptr->Send(msg);
				}
				else
				{
					std::string text = "Client[" + std::to_string(client_ptr->GetID()) + "]: Accept to upload file";
					if (onMessage) onMessage(text);

					reservedSpace += fsize;
					client_ptr->current_file.SetFileSize(fsize);

					ft::Message msg;
					msg.body.clear();
					msg.header.msg_type = ft::MessageType::Accept;
					msg.setString("");
					client_ptr->Send(msg);

					if (onReservedSpaceChanged) onReservedSpaceChanged(reservedSpace);
				}

			});

	}
	else if (msg.header.msg_type == ft::MessageType::FileChunk)
	{

		auto client_ptr = client; 

		auto shared_body = std::make_shared<std::vector<char>>(msg.body);
		auto msg_size = msg.header.size;

		thr_pool.enqueue([this, client_ptr, shared_body, msg_size]() 
			{

				if (!client_ptr->current_file.WriteChunk(shared_body->data(), msg_size))
				{
					if (onMessage) onMessage("cant write chunk");
				}

				ft::Message ack_msg;
				ack_msg.header.msg_type = ft::MessageType::ReceivedChunk;
				ack_msg.setSimpleData(client_ptr->current_file.GetReceivedsize());
				client_ptr->Send(ack_msg);

				if (client_ptr->current_file.IsFileReceived)
				{
					std::string text = "Client[" + std::to_string(client_ptr->GetID()) + "]: Start calculating filehash";
					if (onMessage) onMessage(text);

					if (!client_ptr->current_file.StartCheckHash())
					{
						if (onMessage) onMessage("error while calculating hash");
					}
					else
					{
						std::string text = "Client[" + std::to_string(client_ptr->GetID()) + "]: Finished calculating filehash";
						if (onMessage) onMessage(text);
					}

				}

			});

	}
	else if (msg.header.msg_type == ft::MessageType::ReceivedChunk)
	{
		// delegate task to threads pool
		auto client_ptr = client;
		thr_pool.enqueue([this, client_ptr]()
			{
				// send next chunk
				if (!client_ptr->current_file.SendChunk())
				{
					if (onMessage) onMessage("cant send chunk");
				}
			});
	}
	else if (msg.header.msg_type == ft::MessageType::CalculatingHash)
	{
		std::string text = "Client[" + std::to_string(client->GetID()) + "]: Downloading done. Client start calculating hash";
		if (onMessage) onMessage(text);
	}
	else if (msg.header.msg_type == ft::MessageType::HashFile)
	{
		uint64_t hash_client;
		std::memcpy(&hash_client, msg.body.data(), sizeof(hash_client));

		auto hash_server = hash_files[client->current_file.GetFileID()];
		if (hash_client == hash_server)
		{
			// downloading done
			std::string text = "Client[" + std::to_string(client->GetID()) + "]: Client's hash file correct";
			if (onMessage) onMessage(text);

			goto HashesSimilarOnServer;
		}
		else
		{
			// hashes not equal
			std::string text = "Client[" + std::to_string(client->GetID()) + "]: Client's hash file wrong";
			if (onMessage) onMessage(text);

			temprorary_msg.body.clear();
			temprorary_msg.header.msg_type = ft::MessageType::HashWrong;
			temprorary_msg.setString("");
			MessageClient(client, temprorary_msg);

			client->current_file.Clear();
		}
	}
	else if (msg.header.msg_type == ft::MessageType::HashSimilar)
	{
		std::string text = "Client[" + std::to_string(client->GetID()) + "]: hash files similar";
		if (onMessage) onMessage(text);

		goto HashesSimilarOnClient;
	}
	else if (msg.header.msg_type == ft::MessageType::HashWrong)
	{
		std::string text = "Client[" + std::to_string(client->GetID()) + "]: hash files NOT similar. Wait for re-uploading";
		if (onMessage) onMessage(text);
		client->current_file.Clear();
	}
	else if (msg.header.msg_type == ft::MessageType::HashCalculatingError)
	{
		std::string text = "Client[" + std::to_string(client->GetID()) + "]: client cant calculate hash";
		if (onMessage) onMessage(text);
		client->current_file.Clear();
	}
	else if (msg.header.msg_type == ft::MessageType::FinishedUpload)
	{
	HashesSimilarOnClient:

		// delegate task to threads pool
		auto client_ptr = client;
		thr_pool.enqueue([this, client_ptr]()
			{

				client_ptr->current_file.CloseFile();
				if (!moveDirFromTempToStorage(client_ptr->current_file.GetFileID()))
				{
					std::string text = "Client[" + std::to_string(client_ptr->GetID()) + "]: Error while move file from temp to storage";
					if (onMessage) onMessage(text);

					ft::Message msg;
					msg.body.clear();
					msg.setString("");
					msg.header.msg_type = ft::MessageType::UnknownError;
					client_ptr->Send(msg);

					return;
				}

				RemoveTempFile(client_ptr->current_file.GetFileID());
				AddFile(client_ptr->current_file.GetFileID(), client_ptr->current_file.GetFileSieze(), client_ptr->current_file.GetHash());
				reservedSpace -= client_ptr->current_file.GetFileSieze();
				currentSpace += client_ptr->current_file.GetFileSieze();
				client_ptr->current_file.Clear();

				if (onCurrentSpaceChanged) onCurrentSpaceChanged(currentSpace, storageSpace);
				if (onReservedSpaceChanged) onReservedSpaceChanged(reservedSpace);

				std::string text = "Client[" + std::to_string(client_ptr->GetID()) + "]: Finish uploading";
				if (onMessage) onMessage(text);

				ft::Message msg;
				msg.body.clear();
				msg.setString("");
				msg.header.msg_type = ft::MessageType::FinishedUpload;
				client_ptr->Send(msg);

			});

	}
	else if (msg.header.msg_type == ft::MessageType::FinishedDownload)
	{
	HashesSimilarOnServer:

		std::string text = "Client[" + std::to_string(client->GetID()) + "]: Finish downloading";
		if (onMessage) onMessage(text);

		client->current_file.Clear();

		// send answer to client again
		temprorary_msg.body.clear();
		temprorary_msg.setString("");
		temprorary_msg.header.msg_type = ft::MessageType::FinishedDownload;
		MessageClient(client, temprorary_msg);
	}
	else if (msg.header.msg_type == ft::MessageType::TestMsg)
	{
		std::string test_msg(msg.body.begin(), msg.body.end());
		std::cout << "[" << client->GetID() << "]: " << test_msg << "\n";
	}
}

void CustomServer::ApplySettings()
{
	if (onCurrentSpaceChanged) onCurrentSpaceChanged(0, storageSpace);
	if (onReservedSpaceChanged) onReservedSpaceChanged(0);
	if (onFilesChanged) onFilesChanged(0);
	if (onTempFilesChanged) onTempFilesChanged(0);

	connectedUsers = 0;
	if (onUsersCountChanged) onUsersCountChanged(connectedUsers);

	if (!isStorageCreated)
	{
		
		if (storagePath.empty())
		{
			// default path
			storagePath = ft::fm::GetExecutableDir() + "\\";
		}
		CreateStorage();
	}


	currentSpace = fileStorage.readAll(files, hash_files);
	reservedSpace = fileStorage.readTempAll(temp_files);

	if (onCurrentSpaceChanged) onCurrentSpaceChanged(currentSpace, storageSpace);
	currentFiles = files.size();
	if (onFilesChanged) onFilesChanged(currentFiles);

	if (onReservedSpaceChanged) onReservedSpaceChanged(reservedSpace);
	currentTempFiles = temp_files.size();
	if (onTempFilesChanged) onTempFilesChanged(currentTempFiles);

}

void CustomServer::CreateStorage()
{
	if (!isStorageCreated)
	{
		storagePathFull = storagePath + "Storage\\";
		tempStoragePath = storagePathFull + "temp\\";

		// create folders
		ft::fm::createDir(storagePathFull);
		ft::fm::createDir(tempStoragePath);

		fileStorage.SetPath(storagePathFull); // create files.txt temp_files.txt

		files.clear();
		temp_files.clear();
		hash_files.clear();

		isStorageCreated = true;
	}

}