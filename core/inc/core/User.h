#pragma once

#include "UserInstance.h"
#include <mutex>


class User
{
    using user_logoff_t = std::function<void(std::string /*user_uuid*/)>;
    const std::string m_uuid;
    mutable std::mutex m_instances_mutex;
    std::vector<UserInstance> m_instances;
    user_logoff_t m_on_logoff;
public:
    explicit User(std::string uuid);
    User(User&& r) noexcept;
    bool PostEvent(const event_ptr& event);
    bool PostEventToInstance(const std::string& access_token, const event_ptr& event);
    bool LoginNewInstance(CommonCallData&& data, std::string access_token);
    [[nodiscard]] const std::string& GetUuid() const;
    void ConnectToUserLogoff(user_logoff_t callback);
    bool HasInstance(const std::string& access_token) const;
};