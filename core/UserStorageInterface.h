#pragma once

#include <string>
#include "../grpc/come_together.grpc.pb.h"
struct CommonCallData;
namespace  CT = ComeTogether;

class UserStorageInterface
{
public:
    virtual ~UserStorageInterface() = default;
    //returns access token
    virtual std::string LoginUser(std::string user_uuid) = 0;
    virtual bool PostEventToUser(const std::string& user_uuid, const std::shared_ptr<ComeTogether::event>& evt) = 0;
    virtual bool PostEventToInstance(const std::string& user_uuid, const std::string& access_token, const std::shared_ptr<ComeTogether::event>& evt) = 0;
    virtual void OnNewEventsSubscriber(CommonCallData&& data) = 0;
};