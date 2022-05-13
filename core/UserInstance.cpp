#include "inc/core/UserInstance.h"

#include <plog/Log.h>

UserInstance::UserInstance(CommonCallData&& data, std::string access_token)
    : m_data(std::move(data))
    , m_state(state::IDLE)
    , m_access_token(std::move(access_token))
{}

UserInstance::UserInstance(UserInstance&& r) noexcept
{
    std::lock_guard<decltype(r.m_mtx)> lock{r.m_mtx};
    m_data = std::move(r.m_data);
    m_state = r.m_state.load();
    m_events = std::move(r.m_events);
    m_access_token = std::move(r.m_access_token);
}

UserInstance& UserInstance::operator=(UserInstance&& r) noexcept
{
    if (this != &r)
    {
        std::lock_guard<decltype(m_mtx)> lock1{m_mtx};
        std::lock_guard<decltype(r.m_mtx)> lock2{r.m_mtx};
        m_data = std::move(r.m_data);
        m_state = r.m_state.load();
        m_events = std::move(r.m_events);
        m_access_token = std::move(r.m_access_token);
    }
    return *this;
}

void UserInstance::ConnectToInstanceLogoff(instance_logoff_t callback)
{
    m_on_logoff = std::move(callback);
}

const std::string& UserInstance::GetAccessToken() const
{
    return m_access_token;
}

void UserInstance::SendEvent(const event_ptr& event)
{
    PLOG_INFO << "new event to peer=" << m_data.m_ctx->peer() << ", access_token=" << m_access_token;
    if (m_state.load() == state::SHUTDOWN)
        return;
    if (m_state.exchange(state::WRITING) == state::IDLE)
        m_data.m_writer->Write(*event, this);
    else
    {
        std::lock_guard<decltype(m_mtx)> _{m_mtx};
        m_events.push(event);
    }
}

void UserInstance::OnAsyncEventFinished()
{
    if (m_state.load() == state::SHUTDOWN)
        return;
    std::lock_guard<decltype(m_mtx)> _{m_mtx};
    if (m_events.empty())
        m_state.store(state::IDLE);
    else
    {
        const auto event = std::move(m_events.front());
        m_events.pop();
        m_state.store(state::WRITING);
        m_data.m_writer->Write(*event, this);
    }
}

void UserInstance::OnFinished()
{
    std::unique_lock<decltype(m_mtx)> lock {m_mtx};
    m_state.store(state::SHUTDOWN);
    PLOG_INFO << "peer=" << m_data.m_ctx->peer() << ", access_token=" << m_access_token <<", m_events.size() == " << m_events.size();
    //call unlock because UserInstance will be deleted inside m_on_logoff
    lock.unlock();
    if (m_on_logoff)
        m_on_logoff(m_access_token);
}
