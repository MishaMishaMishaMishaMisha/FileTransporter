#pragma once
#include <string>
#include <fstream>
#include <iostream>
#include "Message.h"
#include "HashFile.h"



namespace ft
{

	class BaseServer;
	class Connection;

	class FileTransferState
	{

	private:

		std::string dir = "";
		std::string filename = "";
		std::string fileID = "";
		uint64_t fileHash = 0;
		uint64_t filesize = 0;
		uint64_t received_size = 0;
		std::ofstream out;
		std::ifstream in;
		uint16_t buffer_size = 4096;
		ft::BaseServer* ref_server = nullptr;
		std::shared_ptr<ft::Connection> ref_client = nullptr;

		ft::Message temproraryMsgOut;

		bool SendFromServerToClient();
		bool SendFromClientToServer();

	public:

		~FileTransferState();

		void Init(const std::string& dir, const std::string& fname, uint64_t fsize = 0, const std::string& fid = "", ft::BaseServer* ref_server = nullptr, std::shared_ptr<ft::Connection> ref_client = nullptr);
		
		void SetFileDir(const std::string& dir);
		void SetFileName(const std::string& fname);
		void SetFileID(const std::string& fid);
		void SetFileHash(const uint64_t fhash);
		void SetRefServer(ft::BaseServer* ref_server);
		void SetRefClient(std::shared_ptr<ft::Connection> ref_client);
		void SetBufferSize(uint16_t buffer_size);
		void SetFileSize(uint64_t size);
		void SetReceivedSize(uint64_t size);
		bool SetInPointer(uint64_t pos);
		bool SetOutPointer(uint64_t pos);

		void CloseFile();
		void Clear();

		uint64_t GetReceivedsize() const { return received_size; }
		uint64_t GetFileSieze() const { return filesize; }
		std::string& GetFileID() { return fileID; }
		std::string& GetFileName() { return filename; }
		std::string& GetDir() { return dir; }
		uint64_t GetHash() { return fileHash; }

		bool StartNewFile();
		bool OpenFile();
		bool SendFileSize();
		bool SendFileName();
		bool SendChunk(bool isFirstChunk = false);
		bool WriteChunk(const char* data, size_t len);

		uint64_t CalculateFileSizeIN();
		uint64_t CalculateFileSizeOUT();

		bool isContinueUploading = false;

		bool IsFileReceived = false;
		bool StartCheckHash();

	};


}

