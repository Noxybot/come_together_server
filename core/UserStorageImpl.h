#pragma once

#include "UserStorageInterface.h"
#include "UserPresenceInterface.h"

#include "User.h"
#include <mutex>

struct CommonCallData;

class UserStorageImpl : public UserStorageInterface, public UserPresenceInterface, public std::enable_shared_from_this<UserStorageImpl>
{
    std::mutex m_mtx;
    std::map<std::string, User> m_users; //user_uuid -> User
    std::map<std::string, CommonCallData> m_connected_devices; //app_id -> CommonCallData
    //std::map<std::string, std::string> m_tokens_waiting_subscription; //access_token -> user_uuid
public:
    void LoginUser(std::string user_uuid, std::string access_token, std::string app_id) override;
    bool PostEventToUser(const std::string& user_uuid, const event_ptr& evt) override;
    bool PostEventToInstance(const std::string& user_uuid, const std::string& access_token,
        const std::shared_ptr<CT::event>& evt) override;
    void OnNewEventsSubscriber(CommonCallData&& data) override;
    bool IsOnline(const std::string& access_token) override;
};
