#pragma once
#include "grpc/come_together.pb.h"

namespace CT = ComeTogether;

class DBInterface
{
public:
    virtual ~DBInterface() = default;
    virtual CT::check_response::result Check(const CT::check_request& req) = 0;
    virtual CT::register_response::result RegisterUser(const CT::register_request& req) = 0;
    virtual CT::login_response::result LoginUser(const CT::login_request& req) = 0;
};