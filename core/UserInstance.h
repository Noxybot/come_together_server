#pragma once
#include "../network/QueueEvents.h"
#include "../network/CommonCallData.h"
#include <atomic>
#include <mutex>
#include <queue>

using event_ptr = std::shared_ptr<come_together_grpc::event>;

class UserInstance : public QueueEvents
{
    using instance_logoff_t = std::function<void(std::string /*access_token*/)>;
    enum class state
    {
        WRITING,
        IDLE,
        SHUTDOWN,
    };
    CommonCallData m_data;
    std::atomic<state> m_state;
    std::mutex m_mtx;
    std::queue<event_ptr> m_events;
    instance_logoff_t m_on_logoff;
public:
    explicit UserInstance(CommonCallData&& data);
    UserInstance(UserInstance&& r) noexcept;
    UserInstance& operator=(UserInstance&& r) noexcept;
    void ConnectToInstanceLogoff(instance_logoff_t callback);
    const std::string& GetAccessToken() const;
    void SendEvent(const event_ptr& event);
    void OnAsyncEventFinished() override;
    void OnFinished() override;
};
