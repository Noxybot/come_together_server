#include "CommonCallData.h"

#include <plog/Log.h>

CommonCallData::CommonCallData()
    : m_ctx(std::make_unique<grpc::ServerContext>())
    , m_token(std::make_unique<come_together_grpc::access_token>())
    , m_writer(std::make_unique<grpc::ServerAsyncWriter<come_together_grpc::event>>(m_ctx.get()))
    , m_state(false)
{}

CommonCallData::CommonCallData(CommonCallData&& r) noexcept
    : m_ctx(std::move(r.m_ctx))
    , m_token(std::move(r.m_token))
    , m_writer(std::move(r.m_writer))
    , m_state(r.m_state.load())
{}

CommonCallData& CommonCallData::operator=(CommonCallData&& r) noexcept
{
    if (this != &r)
    {
        m_ctx = std::move(r.m_ctx);
        m_token = std::move(r.m_token);
        m_writer = std::move(r.m_writer);
        m_state = r.m_state.load();
    }
    return *this;
}

void CommonCallData::ConnectToNewSubscriber(std::function<void(CommonCallData&&)> callback)
{
    PLOG_FATAL_IF(m_state.load()) << "not in initial state";
    if (m_state)
        return;
    m_subscriber_connected = std::move(callback);
}

void CommonCallData::OnAsyncEventFinished()
{
    const bool prev_state = m_state.exchange(true);
    PLOG_FATAL_IF(prev_state == true) << "not in initial state";
    m_subscriber_connected(std::move(*this));
    delete this; //todo: refactor it
}

void CommonCallData::OnFinished()
{
    PLOG_FATAL << "should never be called";
    assert(false);
}
