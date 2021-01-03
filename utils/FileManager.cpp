#include "FileManager.h"
#include "random.h"

#include <algorithm>
#include <cassert>
#include <filesystem>
#include <fstream>

#include <plog/Log.h>

FileManager::FileManager(std::string folder)
    : m_folder(std::move(folder))
{
    PLOG_ERROR_IF(std::filesystem::exists(folder) && !std::filesystem::is_directory(folder))
        << "path=" << folder << " exists and is not directory";
    std::filesystem::create_directory(folder);
}

std::string FileManager::SaveFile(const std::string& file)
{
    const auto file_name = generate_uuid_v4();
    const auto path_to_file = m_folder + file_name;
    std::ofstream f {path_to_file, std::ios_base::binary};
    if (!f.is_open())
    {
        LOG_ERROR << "could not open file=" << path_to_file;
        return {};
    }
    f << file;
    std::lock_guard<decltype(m_mtx)> lock {m_mtx};
    m_cache[file_name] = file;
    PLOG_INFO << "saved file with size=" << file.size() << ", file=" << path_to_file;
    return file_name;
}

std::string FileManager::GetFile(const std::string& file_name)
{
    std::unique_lock<decltype(m_mtx)> lock {m_mtx};
    auto it = m_cache.find(file_name);
    std::string res;
    const auto path_to_file = m_folder + file_name;
    if (it == std::end(m_cache))
    {
        //do not block cache on disk operation
        lock.unlock();
        std::ifstream f {path_to_file, std::ios_base::binary};
        if (!f.is_open())
        {
            LOG_ERROR << "could not open file=" << path_to_file;
            return res;
        }
        res = std::string(std::istreambuf_iterator<char>(f), {});
        lock.lock();
        it = m_cache.find(file_name);
        if (it != std::end(m_cache))
        {
            PLOG_INFO << "someone added file=" << path_to_file << " to cache";
            return it->second;
        }
        m_cache[file_name] = res;
        PLOG_INFO << "added file to cache, file=" << path_to_file << ", size=" << res.size();
    }
    else
    {
         PLOG_INFO << "got file from cache, file=" << path_to_file << ", size=" << it->second.size();
         res = it->second;
    }
    return res;
}

bool FileManager::AddFileToCache(const std::string& file_name)
{
    const auto path_to_file = m_folder + file_name;
    std::unique_lock<decltype(m_mtx)> lock {m_mtx};
    auto it = m_cache.find(file_name);
    if (it != std::end(m_cache))
    {
        PLOG_INFO << "file=" << path_to_file << " is already in cache";
        return true;
    }
    //do not block cache on disk operation
    lock.unlock();
    std::ifstream f { path_to_file, std::ios_base::binary};
    if (!f.is_open())
    {
        LOG_ERROR << "could not open file=" << path_to_file;
        return false;
    }
    std::string content {std::istreambuf_iterator<char>(f), {}};
    PLOG_INFO << "adding file to cache, file=" << path_to_file << ", size=" << content.size();
    lock.lock();
    it = m_cache.find(file_name);
    if (it != std::end(m_cache))
    {
        PLOG_INFO << "someone added file=" << path_to_file << " to cache";
        return true;
    }
    m_cache[file_name] = std::move(content);
    return true;
}