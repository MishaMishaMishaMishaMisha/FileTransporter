#include "FileStorage.h"



void FileStorage::SetPath(const std::string& folderPath) 
{ 
    path = folderPath + "files.txt"; 
    pathTemp = folderPath + "tempFiles.txt";
    CreateFile();
}

void FileStorage::CreateFile()
{
    std::ifstream in(path);
    if (!in.is_open())
    {
        // create file if still not exist
        std::ofstream out(path);
        out.close();
    }
    in.close();

    std::ifstream inTemp(pathTemp);
    if (!inTemp.is_open())
    {
        // create file if still not exist
        std::ofstream outTemp(pathTemp);
        outTemp.close();
    }
    inTemp.close();
}

bool FileStorage::addEntry(const std::string& id, uint64_t size, std::time_t time, uint64_t hash)
{
    std::ofstream out(path, std::ios::app); // add in the end of file
    if (!out.is_open())
    {
        return false;
    }
    out << id << " " << size << " " << time << " " << hash << "\n";
    out.close();
    return true;
}

bool FileStorage::removeEntries(const std::unordered_set<std::string>& idSet)
{
    if (idSet.empty())
    {
        return true;
    }

    std::ifstream in(path);
    if (!in.is_open())
    {
        return false;
    }


    std::ostringstream tempBuffer;
    std::string line;
    size_t removedCount = 0;

    while (std::getline(in, line))
    {
        std::istringstream iss(line);
        std::string currentId;
        uint64_t size;
        std::time_t time;
        uint64_t hash;

        if (!(iss >> currentId >> size >> time >> hash))
        {
            continue; // skip incorrect line
        }

        // save line that dont need delete
        if (idSet.find(currentId) == idSet.end())
        {
            tempBuffer << currentId << " " << size << " " << time << " " << hash << "\n";
        }
        else
        {
            removedCount++;
        }
    }
    in.close();

    std::ofstream out(path, std::ios::trunc);
    out << tempBuffer.str();
    out.close();

    if (removedCount == 0)
    {
        //...
    }

    return true;
}

uint64_t FileStorage::readAll(
    std::map<std::string, std::pair<uint64_t, std::time_t>>& files,
    std::map<std::string, uint64_t>& hash_files)
{
    std::ifstream in(path);
    if (!in.is_open())
    {
        return 0;
    }

    uint64_t totalSize = 0;
    std::string line;

    while (std::getline(in, line))
    {
        std::istringstream iss(line);
        std::string id;
        uint64_t size;
        std::time_t time;
        uint64_t hash;

        if (!(iss >> id >> size >> time >> hash))
        {
            continue;
        }

        files[id] = { size, time };
        hash_files[id] = hash;
        totalSize += size;
    }

    in.close();
    return totalSize;
}


bool FileStorage::addTempEntry(const std::string& id, uint64_t size, std::time_t time)
{
    std::ofstream out(pathTemp, std::ios::app);
    if (!out.is_open())
    {
        return false;
    }
    out << id << " " << size << " " << time << "\n";
    out.close();
    return true;
}

bool FileStorage::removeTempEntries(const std::unordered_set<std::string>& idSet)
{
    if (idSet.empty())
    {
        return true;
    }

    std::ifstream in(pathTemp);
    if (!in.is_open())
    {
        return false;
    }


    std::ostringstream tempBuffer;
    std::string line;
    size_t removedCount = 0;

    while (std::getline(in, line))
    {
        std::istringstream iss(line);
        std::string currentId;
        uint64_t size;
        std::time_t time;

        if (!(iss >> currentId >> size >> time))
        {
            continue;
        }

        // save line than dont need delete
        if (idSet.find(currentId) == idSet.end())
        {
            tempBuffer << currentId << " " << size << " " << time << "\n";
        }
        else
        {
            removedCount++;
        }
    }
    in.close();

    std::ofstream out(pathTemp, std::ios::trunc);
    out << tempBuffer.str();
    out.close();

    if (removedCount == 0)
    {
        // ...
    }

    return true;
}

uint64_t FileStorage::readTempAll(std::map<std::string, std::pair<uint64_t, std::time_t>>& temp_files)

{
    std::ifstream in(pathTemp);
    if (!in.is_open())
    {
        return 0;
    }

    uint64_t totalSize = 0;
    std::string line;

    while (std::getline(in, line))
    {
        std::istringstream iss(line);
        std::string id;
        uint64_t size;
        std::time_t time;

        if (!(iss >> id >> size >> time))
        {
            continue;
        }

        temp_files[id] = { size, time };
        totalSize += size;
    }

    in.close();
    return totalSize;
}