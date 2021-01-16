#pragma once

#include <string>

class UserPresenceInterface
{
public:
    virtual bool IsOnline(const std::string& access_token) = 0;
};