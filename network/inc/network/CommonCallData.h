#pragma once

#include "come_together.grpc.pb.h"
#include "QueueEvents.h"
#include <atomic>
#include <memory>
#include <functional>

struct CommonCallData : private QueueEvents
{
    std::unique_ptr<grpc::ServerContext> m_ctx;
    std::unique_ptr<come_together_grpc::access_token> m_access_token;
    std::unique_ptr<grpc::ServerAsyncWriter<come_together_grpc::event>> m_writer;
    std::atomic<bool> m_state;
    CommonCallData();
    CommonCallData(CommonCallData&& r) noexcept;
    CommonCallData& operator=(CommonCallData&& r) noexcept;
    void ConnectToNewSubscriber(std::function<void(CommonCallData&&)> callback);
private:
    //should be called only once whem users subscribes to events
    void OnAsyncEventFinished() override;
    void OnFinished() override;
    std::function<void(CommonCallData&&)> m_subscriber_connected;
};