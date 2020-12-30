#pragma once

#include "../grpc/come_together.pb.h"

#include <map>
#include <mutex>
#include <string_view>
#include <string>

namespace CT = ComeTogether;

class Mailer
{
    std::mutex m_mtx;
    std::map<std::string, std::string> m_tokens;
    public:
    explicit Mailer(const std::string& from);
    void SendToken(const std::string& to_email);
    CT::verify_token_response::result VerifyToken(const std::string& email, const std::string& token);
};
