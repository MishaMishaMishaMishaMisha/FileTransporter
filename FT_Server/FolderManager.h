#pragma once

#include <string>
#include <map>
#include <chrono>
#include <cstdint>
#include <iostream>

#include <HashFile.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <cstdio>
#include <cstdlib>
#include <sys/statvfs.h>
#endif

namespace ft
{

	namespace fm
	{
		bool IsPathValid(const std::string& path);
		uint64_t GetFreeDiskSpace(const std::string& DiskPath);

		bool createDir(const std::string& name);
		bool removeDir(const std::string& name);
		bool moveDir(const std::string& src, const std::string& dst);
		bool FindFileByDirName(std::string& path, std::string& name, std::string& f_name); // if folder exist, find file in folder, set f_name with filename 
		uint64_t GetDirSize(std::string& path, int depth = 0);
		time_t FileTimeToTimeT(const FILETIME& ft);
		void GetFoldersInfo(std::map<std::string, std::pair<uint64_t, std::time_t>>& files, 
							std::map<std::string, uint64_t>& hash_files, 
							const std::string& folderPath, 
							const std::string& ignoreName = "");
		std::string GetExecutableDir();

	}

}

