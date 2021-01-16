#include "UserStorageImpl.h"
#include "User.h"

#include "../network/CommonCallData.h"
#include "../utils/random.h"

#include <plog/Log.h>

void UserStorageImpl::LoginUser(std::string user_uuid, std::string access_token, std::string app_id)
{
    std::lock_guard<decltype(m_mtx)> lock {m_mtx};
    auto connected_device = m_connected_devices.find(app_id);
    if (connected_device != std::end(m_connected_devices))
    {
        auto& user = m_users.emplace(user_uuid, user_uuid).first->second;
        user.LoginNewInstance(std::move(connected_device->second), std::move(access_token));
        m_connected_devices.erase(connected_device);
        user.ConnectToUserLogoff([this](std::string user_uuid)
            {
                std::lock_guard<decltype(m_mtx)> lock {m_mtx};
                const auto user_it = m_users.find(user_uuid);
                if (user_it != std::end(m_users))
                    m_users.erase(user_it);
            });
    }
}

bool UserStorageImpl::PostEventToUser(const std::string& user_uuid, const event_ptr& evt)
{
    std::lock_guard<decltype(m_mtx)> lock {m_mtx};
    const auto user_it = m_users.find(user_uuid);
    if (user_it != std::end(m_users))
        return user_it->second.PostEvent(evt);
    return false;
}

void UserStorageImpl::OnNewEventsSubscriber(CommonCallData&& data)
{
    auto app_id = data.m_application_id->id();
    std::lock_guard<decltype(m_mtx)> lock {m_mtx};
    m_connected_devices.emplace(std::move(app_id), std::move(data));
}

bool UserStorageImpl::IsOnline(const std::string& access_token)
{
    std::lock_guard<decltype(m_mtx)> lock {m_mtx};
    return false;
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
