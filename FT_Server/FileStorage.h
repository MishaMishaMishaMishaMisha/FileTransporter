#pragma once

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <ctime>
#include <cstdint>
#include <unordered_set>

class FileStorage
{
private:
    std::string path;
    std::string pathTemp;

public:

    void SetPath(const std::string& folderPath);

    void CreateFile();

    bool addEntry(const std::string& id, uint64_t size, std::time_t time, uint64_t hash);

    bool removeEntries(const std::unordered_set<std::string>& idSet);

    uint64_t readAll(
        std::map<std::string, std::pair<uint64_t, std::time_t>>& files,
        std::map<std::string, uint64_t>& hash_files);


    bool addTempEntry(const std::string& id, uint64_t size, std::time_t time);

    bool removeTempEntries(const std::unordered_set<std::string>& idSet);

    uint64_t readTempAll(std::map<std::string, std::pair<uint64_t, std::time_t>>& temp_files);
};

