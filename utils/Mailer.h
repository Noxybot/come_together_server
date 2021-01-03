#pragma once

#include "MailerInteface.h"

#include <map>
#include <mutex>
#include <string_view>
#include <string>

class Mailer : public MailerInterface
{
    std::mutex m_mtx;
    std::map<std::string, std::string> m_tokens;
    const std::string m_from;
    const std::string m_login;
    const std::string m_password;
    public:
    explicit Mailer(std::string from, std::string login, std::string password);
    CT::ask_token_response_result SendToken(const std::string& to_email) override;
    CT::verify_token_response_result VerifyToken(const std::string& email, const std::string& token) override;
private:
    bool SendTokenImpl(std::string to, std::string token);
};
