#pragma once

class QueueEvents
{
public:
    virtual ~QueueEvents() = default;
    virtual void OnAsyncEventFinished() = 0;
    virtual void OnFinished() = 0;
};
