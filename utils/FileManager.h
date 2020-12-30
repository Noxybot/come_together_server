#pragma once
#include <string>
#include <map>
#include <mutex>

class FileManager
{
    std::map<std::string, std::string> m_cache;
    mutable std::mutex m_mtx;
public:
    //returns random file_name
    explicit FileManager();
    std::string SaveFile(const std::string& image);
    std::string GetFile(const std::string& file_name);
};
