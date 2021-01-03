#pragma once
#include <string>
#include <map>
#include <mutex>

class FileManager
{
    std::map<std::string, std::string> m_cache;
    std::mutex m_mtx;
    const std::string m_folder;
public:
    explicit FileManager(std::string folder);
    //returns random file_name
    std::string SaveFile(const std::string& image);
    std::string GetFile(const std::string& file_name);
    bool AddFileToCache(const std::string& file_name);
};
