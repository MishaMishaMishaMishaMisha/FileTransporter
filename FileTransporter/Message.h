#pragma once
#include <iostream>
#include <vector>


namespace ft
{

	enum class MessageType
	{
        UploadFile,
        ContinueUploadFile,
        DownloadFile,
        FileNotExist,
        NotEnoughSpace,
        Accept,
        AcceptContinue,
        CancelTransfer,
		FileName,
		FileSize,
        FileTooBig,
		FileChunk,
        FileID,
        GetWholeFile,
        GetPartFile,
        TestMsg,
        ReceivedChunk,
        CalculatingHash,
        CalculatingHashDone,
        HashFile,
        HashSimilar,
        HashWrong,
        HashCalculatingError,
        WrongFile,
        FinishedUpload,
        FinishedDownload,
        UnknownError,
		NoneType
	};

    struct MessageHeader
    {
        MessageType msg_type = MessageType::NoneType;
        uint64_t size = 0; // body_size
    };

	struct Message
	{
        MessageHeader header{};
		std::vector<char> body;

        template<typename T>
        void setSimpleData(const T& data)
        {
            body.resize(sizeof(T));
            std::memcpy(body.data(), &data, sizeof(T));
            header.size = static_cast<uint64_t>(body.size());
        }

        void setString(const std::string& str)
        {
            body.assign(str.begin(), str.end());
            header.size = static_cast<uint64_t>(body.size());
        }

        void setBytes(const char* data, size_t len)
        {
            body.assign(data, data + len);
            header.size = static_cast<uint64_t>(body.size());
        }

	};



    class Connection;

    struct OwnedMessage
    {
        std::shared_ptr<ft::Connection> remote = nullptr;
        ft::Message msg;
    };

}