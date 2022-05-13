#pragma once

#include "database/DBInterface.h"

#include <map>
#include <memory>
#include <mutex>

class PushSender
{
    const DBInterface::Ptr m_db;
    std::multimap<std::string, std::string> m_push_tokens;
    std::mutex m_mtx;
public:
    explicit PushSender(DBInterface::Ptr db);
};