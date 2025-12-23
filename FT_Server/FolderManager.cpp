#include "FolderManager.h"




bool ft::fm::IsPathValid(const std::string& path)
{

	std::string parent = path;

	size_t pos = parent.find_last_of("/\\");
	if (pos != std::string::npos)
	{
		parent = parent.substr(0, pos);
	}
	else
	{
		parent = ".";
	}

#ifdef _WIN32
	DWORD attrs = GetFileAttributesA(parent.c_str());
	if (attrs == INVALID_FILE_ATTRIBUTES)
	{
		return false;
	}
	if (!(attrs & FILE_ATTRIBUTE_DIRECTORY))
	{
		return false;
	}


	HANDLE hFile = CreateFileA(
		(parent + "\\__tmp_test_perm__.tmp").c_str(),
		GENERIC_WRITE,
		0,
		NULL,
		CREATE_NEW,
		FILE_ATTRIBUTE_TEMPORARY,
		NULL
	);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		if (GetLastError() == ERROR_FILE_EXISTS)
		{
			return true;
		}

		return false;
	}

	CloseHandle(hFile);
	DeleteFileA((parent + "\\__tmp_test_perm__.tmp").c_str());
	return true;

#else
	struct stat st;
	if (stat(parent.c_str(), &st) != 0)
	{
		return false;
	}
	if (!S_ISDIR(st.st_mode))
	{
		return false;
	}

	if (access(parent.c_str(), W_OK) != 0)
	{
		return false;
	}

	return true;
#endif
}

uint64_t ft::fm::GetFreeDiskSpace(const std::string& DiskPath)
{

	std::string path;

#ifdef _WIN32
	ULARGE_INTEGER freeBytesAvailable, totalBytes, totalFreeBytes;

	if (DiskPath.empty())
	{
		path = "C:\\"; // default disk
	}
	else
	{
		path = DiskPath;
	}

	BOOL result = GetDiskFreeSpaceExA(
		path.c_str(),
		&freeBytesAvailable,
		&totalBytes,
		&totalFreeBytes
	);

	if (result)
	{
		return static_cast<uint64_t>(freeBytesAvailable.QuadPart);
	}
	else
	{
		return 0;
	}

#else // POSIX (Linux, macOS, etc.)

	if (DiskPath.empty())
	{
		path = "/";
	}
	else
	{
		path = DiskPath;
	}

	struct statvfs stat;
	if (statvfs(path.c_str(), &stat) != 0)
	{
		return 0;
	}

	return static_cast<uint64_t>(stat.f_bsize) * static_cast<uint64_t>(stat.f_bavail);
#endif
}


bool ft::fm::createDir(const std::string& name)
{
#ifdef _WIN32
	return CreateDirectoryA(name.c_str(), NULL) || GetLastError() == ERROR_ALREADY_EXISTS;
#else
	return mkdir(name.c_str(), 0777) == 0 || errno == EEXIST;
#endif
}

bool ft::fm::removeDir(const std::string& name)
{
#ifdef _WIN32
	WIN32_FIND_DATAA findFileData;
	HANDLE hFind = INVALID_HANDLE_VALUE;

	std::string dirSpec = name + "\\*";
	hFind = FindFirstFileA(dirSpec.c_str(), &findFileData);

	if (hFind == INVALID_HANDLE_VALUE)
	{
		return false;
	}

	do
	{
		const char* sub = findFileData.cFileName; // sub folder or file

		if (strcmp(sub, ".") == 0 || strcmp(sub, "..") == 0)
		{
			continue;
		}

		std::string fullPath = name + "\\" + sub;

		if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			removeDir(fullPath);
		}
		else
		{
			DeleteFileA(fullPath.c_str());
		}

	} while (FindNextFileA(hFind, &findFileData) != 0);

	FindClose(hFind);

	return RemoveDirectoryA(name.c_str());
#else
	DIR* dir = opendir(path.c_str());
	if (!dir) return false;

	struct dirent* entry;
	while ((entry = readdir(dir)) != nullptr)
	{
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
		{
			continue;
		}

		std::string fullPath = path + "/" + entry->d_name;

		if (entry->d_type == DT_DIR)
		{
			removeDirectory(fullPath);
		}
		else
		{
			unlink(fullPath.c_str());
		}
	}

	closedir(dir);
	return (rmdir(path.c_str()) == 0);
#endif
}

bool ft::fm::moveDir(const std::string& src, const std::string& dst)
{
#ifdef _WIN32
	if (MoveFileExA(src.c_str(), dst.c_str(), MOVEFILE_COPY_ALLOWED | MOVEFILE_WRITE_THROUGH))
	{
		return true;
	}
	else
	{
		DWORD err = GetLastError();
		return false;
	}
#else
	if (rename(src.c_str(), dst.c_str()) == 0)
	{
		return true;
	}
	else
	{
		int err = errno;
		return false;
	}
#endif
}

bool ft::fm::FindFileByDirName(std::string& path, std::string& name, std::string& f_name)
{
#ifdef _WIN32
	std::string full_path = path + name + "\\*";

	WIN32_FIND_DATAA findData;
	HANDLE hFind = FindFirstFileA(full_path.c_str(), &findData);

	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (!(findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY))
			{
				f_name = findData.cFileName;
				FindClose(hFind);
				return true;
			}
		} while (FindNextFileA(hFind, &findData));
		FindClose(hFind);
	}
	else
	{
		return false;
	}
#else
	std::string full_path = path + name;
	DIR* dir = opendir(full_path.c_str());
	if (!dir)
	{
		return false;
	}

	struct dirent* entry;
	while ((entry = readdir(dir)) != nullptr)
	{
		std::string filename = entry->d_name;
		if (filename != "." && filename != "..")
		{
			f_name = filename;
			return true;
		}
	}
	closedir(dir);
	return false;
#endif
}

uint64_t ft::fm::GetDirSize(std::string& path, int depth)

{

	uint64_t totalSize = 0;

#ifdef _WIN32
	WIN32_FIND_DATAA findFileData;
	HANDLE hFind = INVALID_HANDLE_VALUE;

	std::string searchPath = path + "\\*";
	hFind = FindFirstFileA(searchPath.c_str(), &findFileData);

	if (hFind == INVALID_HANDLE_VALUE)
		return 0;

	do
	{
		const char* name = findFileData.cFileName;

		if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
			continue;

		std::string fullPath = path + "\\" + name;

		if (findFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) 
		{
			totalSize += GetDirSize(fullPath, depth + 1);
		}
		else 
		{
			LARGE_INTEGER fileSize;
			fileSize.HighPart = findFileData.nFileSizeHigh;
			fileSize.LowPart = findFileData.nFileSizeLow;
			totalSize += fileSize.QuadPart;
		}

	} while (FindNextFileA(hFind, &findFileData) != 0);

	FindClose(hFind);

#else // Linux / Unix
	DIR* dir = opendir(path.c_str());
	if (!dir) return 0;

	struct dirent* entry;
	while ((entry = readdir(dir)) != nullptr) {
		if (std::string(entry->d_name) == "." || std::string(entry->d_name) == "..")
			continue;

		std::string fullPath = path + "/" + entry->d_name;
		struct stat st;

		if (stat(fullPath.c_str(), &st) == 0) {
			if (S_ISDIR(st.st_mode)) {
				totalSize += GetDirSize(fullPath, depth + 1);
			}
			else {
				totalSize += st.st_size;
			}
		}
	}
	closedir(dir);
#endif

	return totalSize;
}

time_t ft::fm::FileTimeToTimeT(const FILETIME& ft)
{
	ULARGE_INTEGER ull;
	ull.LowPart = ft.dwLowDateTime;
	ull.HighPart = ft.dwHighDateTime;

	const unsigned long long EPOCH_DIFF = 116444736000000000ULL;
	if (ull.QuadPart < EPOCH_DIFF) return 0;

	return static_cast<time_t>((ull.QuadPart - EPOCH_DIFF) / 10000000ULL);
}

void ft::fm::GetFoldersInfo(std::map<std::string, std::pair<uint64_t, std::time_t>>& files,
							std::map<std::string, uint64_t>& hash_files,
							const std::string& folderPath,
							const std::string& ignoreName) 
{
#ifdef _WIN32
	std::string searchPath = folderPath + "\\*";
	WIN32_FIND_DATAA findData;
	HANDLE hFind = FindFirstFileA(searchPath.c_str(), &findData);

	if (hFind == INVALID_HANDLE_VALUE)
	{
		return;
	}

	do 
	{
		if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) 
		{
			std::string folderName = findData.cFileName;

			if (folderName == "." || folderName == "..") continue;
			if (!ignoreName.empty() && folderName == ignoreName) continue;

			std::string fullPath = folderPath + "\\" + folderName;

			uint64_t size = GetDirSize(fullPath);
			time_t creationTime = FileTimeToTimeT(findData.ftCreationTime);

			files[folderName] = { static_cast<uint32_t>(size), creationTime };


			std::string innerSearch = fullPath + "\\*";
			WIN32_FIND_DATAA innerFind;
			HANDLE hInner = FindFirstFileA(innerSearch.c_str(), &innerFind);

			if (hInner != INVALID_HANDLE_VALUE) 
			{
				do 
				{
					if (!(innerFind.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) 
					{
						std::string fileName = innerFind.cFileName;
						std::string filePath = fullPath + "\\" + fileName;

						hash_files[folderName] = ft::filehash::CalculateSimpleFileHash(filePath);
						break;
					}
				} while (FindNextFileA(hInner, &innerFind));
				FindClose(hInner);
			}
		}
	} while (FindNextFileA(hFind, &findData));

	FindClose(hFind);

#else
	DIR* dir = opendir(folderPath.c_str());
	if (!dir) return;

	struct dirent* entry;
	while ((entry = readdir(dir)) != nullptr) 
	{
		if (entry->d_type == DT_DIR) {
			std::string folderName = entry->d_name;

			if (folderName == "." || folderName == "..") continue;
			if (!ignoreName.empty() && folderName == ignoreName) continue;

			std::string fullPath = folderPath + "/" + folderName;

			struct stat st;
			if (stat(fullPath.c_str(), &st) == 0) 
			{
				uint64_t size = GetDirSize(fullPath);
				time_t creationTime = st.st_ctime;

				files[folderName] = { static_cast<uint32_t>(size), creationTime };

				DIR* innerDir = opendir(fullPath.c_str());
				if (innerDir) {
					struct dirent* innerEntry;
					while ((innerEntry = readdir(innerDir)) != nullptr) 
					{
						if (innerEntry->d_type == DT_REG) 
						{ 
							std::string fileName = innerEntry->d_name;
							std::string filePath = fullPath + "/" + fileName;

							hash_files[folderName] = CalculateSimpleFileHash(filePath);
							break;
						}
					}
					closedir(innerDir);
				}
			}
		}
	}
	closedir(dir);
#endif
}

std::string ft::fm::GetExecutableDir()
{
	char buffer[MAX_PATH];
	GetModuleFileNameA(nullptr, buffer, MAX_PATH); 
	std::string fullPath = buffer;

	size_t pos = fullPath.find_last_of("\\/");
	if (pos != std::string::npos)
	{
		return fullPath.substr(0, pos);
	}

	return fullPath;
}

