#pragma once


#include <BaseClient.h>
#include <cctype>
#include <algorithm>
#include <iomanip>   // for std::setprecision
#include <queue>


class CustomClient : public ft::BaseClient
{
public:

	CustomClient() {}
	virtual ~CustomClient();

	void SetDownloadsPath(std::string& path) { pathForDownloads = path; }
	bool UploadFile(std::string& file);
	void ContinueUploadFile(std::string& id);
	void DownloadFile(std::string& id);

	void StartThreadHandleMessages();
	void StopThreadHandleMessages();

	void CancelTranseringFile(); // cancel all operations

	bool CheckFile(const std::string& file);

	// callback to print info in GUI
	std::function<void(const std::string&)> onMessage;
	// callback to notice about finish work (upload/download)
	std::function<void()> onFinished;
	// callback to get work progress
	std::function<void(const uint64_t currentValue, const uint64_t maxValue)> onProgress;

	// callback to notice about connection lost
	void setOnConnectionLostFunction(std::function<void()> f) 
	{
		if (connection)
		{
			connection->onConnectionLost = f;
		}
	}

private:

	void HandleReceivedMsg(); // read messages 
	void ProcessMessages();   // execute tasks of messages

	void progress();
	uint64_t currentValue = 0;
	uint64_t MaxValue = 0;

	std::ifstream in;
	ft::Message temprorary_msg;
	std::string pathForDownloads = "";

	std::atomic<bool> running{ false };


	// threads
	std::thread thr_reader;
	std::thread thr_processor;

	std::queue<ft::Message> m_processingQueue;

	std::mutex m_processingMutex;
	std::condition_variable m_processingCV;

	std::atomic<bool> IsCancelTranseringFile;

};