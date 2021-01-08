#pragma once

#include <string>
#include "../grpc/come_together.pb.h"

namespace  CT = come_together_grpc;

class MailerInterface
{
public:
    virtual ~MailerInterface() = default;
    virtual CT::ask_token_response_result SendToken(const std::string& to_email) = 0;
    virtual CT::verify_token_response_result VerifyToken(const std::string& email, const std::string& token) = 0;
};