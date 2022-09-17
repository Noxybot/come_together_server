#include "inc/core/User.h"

#include "network/CommonCallData.h"

#include <atomic>
#include <queue>

#include <plog/Log.h>

User::User(std::string uuid)
    : m_uuid(std::move(uuid))
{}

User::User(User&& r) noexcept
    : m_uuid(r.m_uuid)
{
    std::lock_guard<decltype(r.m_instances_mutex)> _ {r.m_instances_mutex};
    m_instances = std::move(r.m_instances);
}

bool User::PostEvent(const event_ptr& event)
{
    std::lock_guard<decltype(m_instances_mutex)> _ {m_instances_mutex};
    for (auto& instance : m_instances)
        instance.SendEvent(event);
    return !m_instances.empty();
}

bool User::PostEventToInstance(const std::string& access_token, const event_ptr& event)
{
    std::lock_guard<decltype(m_instances_mutex)> _ {m_instances_mutex};
    for (auto& instance : m_instances)
    {
        if (instance.GetAccessToken() == access_token)
        {
            instance.SendEvent(event);
            return true;
        }
    }
    return false;
}

bool User::LoginNewInstance(CommonCallData&& data, std::string access_token)
{
    std::lock_guard<decltype(m_instances_mutex)> _ {m_instances_mutex};
   const auto it = std::find_if(m_instances.begin(), m_instances.end(),
        [&](const UserInstance& instance_) {return instance_.GetAccessToken() == access_token; });
    if (it != std::end(m_instances))
    {
        PLOG_INFO << "there is instance with access token " << access_token << ", deleting it";
        m_instances.erase(it);
    }
    auto& instance = m_instances.emplace_back(std::move(data), std::move(access_token));
    instance.ConnectToInstanceLogoff(
        [this](std::string access_token) 
    {
         std::unique_lock<decltype(m_instances_mutex)> lock {m_instances_mutex};
         const auto it = std::find_if(m_instances.begin(), m_instances.end(),
             [&](const UserInstance& instance_){return instance_.GetAccessToken() == access_token;});
         PLOG_ERROR_IF(it == std::end(m_instances)) << "instance not found";
         m_instances.erase(it);
         lock.unlock();
         PLOG_INFO << "instance with access_token=" << access_token << " logoff";
         if (m_instances.empty() && m_on_logoff)
             m_on_logoff(m_uuid);
    });
    return true;
}

const std::string& User::GetUuid() const
{
    return m_uuid;
}

void User::ConnectToUserLogoff(user_logoff_t callback)
{
    m_on_logoff = std::move(callback);
}

bool User::HasInstance(const std::string& access_token) const
{
    std::lock_guard lock { m_instances_mutex };
    for (const auto& instance : m_instances)
    {
        if (instance.GetAccessToken() == access_token)
            return true;
    }
    return false;
}
