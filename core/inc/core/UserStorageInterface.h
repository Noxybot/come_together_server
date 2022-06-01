#pragma once

#include <string>
#include "come_together.grpc.pb.h"
struct CommonCallData;
namespace  CT = come_together_grpc;

class UserStorageInterface
{
public:
    virtual ~UserStorageInterface() = default;
    //returns access token
    virtual void LoginUser(std::string user_uuid, std::string access_token, std::string app_id) = 0;
    virtual void PostToAllUsers(const std::shared_ptr<CT::event>& evt) = 0;
    virtual bool PostEventToUser(const std::string& user_uuid, const std::shared_ptr<CT::event>& evt) = 0;
    virtual bool PostEventToInstance(const std::string& user_uuid, const std::string& access_token, const std::shared_ptr<CT::event>& evt) = 0;
    virtual void OnNewEventsSubscriber(CommonCallData&& data) = 0;
};