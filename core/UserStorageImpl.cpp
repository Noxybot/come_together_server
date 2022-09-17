#include "inc/core/UserStorageImpl.h"
#include "inc/core/User.h"

#include "network/CommonCallData.h"
#include "utils/random.h"


#include <plog/Log.h>


void UserStorageImpl::LoginUser(std::string user_uuid, std::string access_token, std::string app_id)
{
    std::lock_guard lock {m_mtx};
    auto connected_device = m_connected_devices.find(app_id);
    if (connected_device != std::end(m_connected_devices))
    {
        auto& user = m_users.emplace(user_uuid, user_uuid).first->second;
        user.LoginNewInstance(std::move(connected_device->second), std::move(access_token));
        m_connected_devices.erase(connected_device);
        user.ConnectToUserLogoff([this](std::string user_uuid)
            {
                std::lock_guard<decltype(m_mtx)> lock{ m_mtx };
                const auto user_it = m_users.find(user_uuid);
                if (user_it != std::end(m_users))
                    m_users.erase(user_it);
            });
    }
    else
        PLOG_INFO << "device not found";
}

void UserStorageImpl::PostToAllUsers(const std::shared_ptr<CT::event>& evt)
{
    std::lock_guard<decltype(m_mtx)> lock {m_mtx};
    for (auto& user : m_users)
        user.second.PostEvent(evt);
}

bool UserStorageImpl::PostEventToUser(const std::string& user_uuid, const event_ptr& evt)
{
    std::lock_guard<decltype(m_mtx)> lock {m_mtx};
    const auto user_it = m_users.find(user_uuid);
    if (user_it != std::end(m_users))
    {
        PLOG_INFO << "found user " << user_uuid << ", posting event to him";
        return user_it->second.PostEvent(evt);
    }
    else
        PLOG_INFO << "user " << user_uuid << " not found, event will be lost";
    return false;
}

void UserStorageImpl::OnNewEventsSubscriber(CommonCallData&& data)
{
    const auto& app_id = data.m_access_token->value();
    PLOG_INFO << "new subscriber app id: " << app_id;
    {
        std::lock_guard lock{ m_mtx };
        m_connected_devices.emplace(app_id, std::move(data));
    }
    auto uuid = m_get_uuid_by_app_id(app_id);
    if (!uuid.empty())
    {
        PLOG_INFO << "user: " << uuid << " already logged in";
        LoginUser(uuid, app_id, app_id);
    }
}

bool UserStorageImpl::IsOnline(const std::string& access_token)
{
    std::lock_guard lock {m_mtx};
    return false;
}

std::string UserStorageImpl::GetUuidByAccessToken(const std::string& access_token)
{
    std::lock_guard lock{ m_mtx };
    for (const auto& user : m_users)
    {
        if (user.second.HasInstance(access_token))
            return user.second.GetUuid();
    }
    return {};
}

bool UserStorageImpl::PostEventToInstance(const std::string& user_uuid, const std::string& access_token,
                                          const std::shared_ptr<CT::event>& evt)
{
    std::lock_guard<decltype(m_mtx)> lock {m_mtx};
    const auto user_it = m_users.find(user_uuid);
    if (user_it ==  std::end(m_users))
    {
        LOG_WARNING << "user_uuid=" << user_uuid << " not found";
        return false;
    }
    return user_it->second.PostEventToInstance(access_token, evt);
}

void UserStorageImpl::SetGetUuidByAppId(get_uuid_t fun)
{
    m_get_uuid_by_app_id = std::move(fun);
}

