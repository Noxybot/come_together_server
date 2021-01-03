#include "UserStorageImpl.h"
#include "User.h"

#include "../network/CommonCallData.h"
#include "../utils/random.h"

#include <plog/Log.h>

std::string UserStorageImpl::LoginUser(std::string user_uuid)
{
    auto token = get_random_string(16, random_string_type::alphabetic);
    std::lock_guard<decltype(m_mtx)> _ {m_mtx};
    const auto res = m_tokens_waiting_subscription.emplace(token, std::move(user_uuid));
    PLOG_ERROR_IF(!res.second) << "access_token=" << token << " already exists in m_tokens_waiting_subscription";
    return token;
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
    auto& token = data.m_token->token();
    std::lock_guard<decltype(m_mtx)> lock {m_mtx};
    const auto token_it = m_tokens_waiting_subscription.find(token);
    PLOG_ERROR_IF(token_it == std::end(m_tokens_waiting_subscription)) << "access_token=" << token << " not found";
    if (token_it != std::end(m_tokens_waiting_subscription))
    {
        const auto& user_uuid = token_it->second;
        const auto user_it = m_users.find(user_uuid);
        if (user_it == std::end(m_users))
        {
            const auto emplace_res = m_users.emplace(user_uuid, user_uuid);
            auto& user = emplace_res.first->second;
            user.LoginNewInstance(std::move(data));
            user.ConnectToUserLogoff([this](std::string user_uuid)
            {
                std::lock_guard<decltype(m_mtx)> lock {m_mtx};
                const auto user_it = m_users.find(user_uuid);
                if (user_it != std::end(m_users))
                    m_users.erase(user_it);
            });
        }
        else
            user_it->second.LoginNewInstance(std::move(data));
        m_tokens_waiting_subscription.erase(token_it);
    }
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
