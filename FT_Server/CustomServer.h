#pragma once

#include <BaseServer.h>
#include "FolderManager.h"
#include "FileStorage.h"
#include "ThreadPool.h"

#include <random>
#include <map>
#include <chrono>
#include <sstream>



class CustomServer : public ft::BaseServer
{
public:

	CustomServer(const uint16_t port);

	virtual ~CustomServer();

	void setStoragePath(std::string& path);
	void setStorageSpace(int gb);
	void setMaxUserFileSize(int gb);
	void setCleaningEnable(bool enable) { isCleaningEnable = enable; }
	void setMaxTimeFileExisting(uint32_t seconds) { max_time_file_existing = seconds; }
	void setMaxTimeTempFileExisting(uint32_t seconds) { max_time_tempFile_existing = seconds; }

	std::string getStoragePath() { return storagePath; }
	int getStorageSpace() { return storageSpaceGB; }
	int getMaxUserFileSize() { return max_user_filesize_GB; }
	bool getCleaningEnable() { return isCleaningEnable; }
	uint32_t getMaxTimeFileExisting() { return max_time_file_existing; }
	uint32_t getMaxTimeTempFileExisting() { return max_time_tempFile_existing; }
	uint64_t getCurrentSpace() { return currentSpace; }

	void ApplySettings();


	void CallCheckConnectionMethod() { CheckConnections(); }
	void CleanStorage(); // delete old files (>1month by default)
	void CleanTemp(); // delete old temp files (>1day by default)
	void CleanFullStorage(); // delete all files and temp files in storage


	// callback to print info in GUI
	std::function<void(const std::string&)> onMessage;
	// callback to get amount of connected users
	std::function<void(const uint32_t)> onUsersCountChanged;
	// callback when free space changed
	std::function<void(const uint64_t, const uint64_t)> onCurrentSpaceChanged;
	// callback when reserved space changed
	std::function<void(const uint64_t)> onReservedSpaceChanged;
	// callback when amount of files changed
	std::function<void(const uint32_t)> onFilesChanged;
	// callback when amount of temp files changed
	std::function<void(const uint32_t)> onTempFilesChanged;


protected:

	virtual bool OnClientConnect(std::shared_ptr<ft::Connection> client) override;
	virtual void OnClientDisconnect(std::shared_ptr<ft::Connection> client) override;
	virtual void OnClientValidated(std::shared_ptr<ft::Connection> client) override;
	virtual void OnMessage(std::shared_ptr<ft::Connection> client, const ft::Message& msg) override;
	virtual void OnServerStarted() override;
	virtual void OnServerStopped() override;


private:

	FileStorage fileStorage;

	uint32_t connectedUsers = 0;

	std::string storagePath = "";
	std::string storagePathFull = "Storage\\";
	std::string tempStoragePath = storagePathFull + "temp\\";

	uint32_t storageSpaceGB = 5; 
	uint64_t storageSpace = static_cast<uint64_t>(storageSpaceGB) * 1024 * 1024 * 1024; // default 5 GB
	uint64_t currentSpace = 0;
	uint64_t reservedSpace = 0;

	uint32_t currentFiles = 0;
	uint32_t currentTempFiles = 0;

	uint32_t max_user_filesize_GB = 1;
	uint64_t max_user_filesize = max_user_filesize_GB * 1024 * 1024 * 1024; // default 1 GB

	bool isCleaningEnable = true;

	bool isStorageCreated = false;

	std::map<std::string, std::pair<uint64_t, std::time_t>> files; // client's files: <id, <size, uploaded_time>>
	std::mutex mtx;
	uint32_t max_time_file_existing = 30 * 24 * 60 * 60; // default 1 month

	std::map<std::string, std::pair<uint64_t, std::time_t>> temp_files; // client's files before full upload
	std::mutex mtx_temp;
	uint32_t max_time_tempFile_existing = 1 * 24 * 60 * 60; // default 1 day

	std::map<std::string, uint64_t> hash_files; // client's file hashes: <id, hash>


	size_t num_threads = 10;
	ThreadPool thr_pool;


	bool moveDirFromTempToStorage(const std::string& name);
	std::string generateID();
	void AddFile(std::string& fileID, uint64_t fileSize, uint64_t hash);
	void AddTempFile(std::string& fileID, uint64_t fileSize);
	void RemoveFile(std::string& fileID);
	void RemoveTempFile(std::string& fileID);

	void CreateStorage();

};
