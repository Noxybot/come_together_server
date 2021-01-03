#include "Mailer.h"
#include <random>
#include "random.h"
#include <algorithm>
#include <cstddef>
#include <cstring>
#include <curl/curl.h>
#include <fmt/format.h>

#include <plog/Log.h>

static const char *payload_text[] = {
    "Date: Mon, 29 Nov 2010 21:54:29 +1100\r\n",
    "To: {to_tag} \r\n",
    "From: ComeTogether {from_tag} \r\n",
    "Message-ID: <dcd7cb36-11db-487a-9f3a-e652a9458efd>",
    "rfcpedant.example.org>\r\n",
    "Subject: ComeTogether email verification {secret_tag}\r\n",
    "\r\n", /* empty line to divide headers from body, see RFC5322 */
    "Token: {secret_tag}\r\n",
    nullptr
};
struct upload_status
{
  int lines_read = 0;
  std::string to, secret, from;
};

static size_t payload_source(void *ptr, size_t size, size_t nmemb, void *userp)
{
    const auto upload_ctx = static_cast<upload_status *>(userp);
    if((size == 0) || (nmemb == 0) || ((size*nmemb) < 1))
        return 0;
    const char* data = payload_text[upload_ctx->lines_read];
    if (!data)
      return 0;
    auto test = std::string {data};
    if (test.find("to_tag") != std::string::npos)
    {
        test = fmt::format(test, fmt::arg("to_tag", upload_ctx->to));
        data = test.c_str();
    }
    else if (test.find("secret_tag") != std::string::npos)
    {
        test = fmt::format(test, fmt::arg("secret_tag", upload_ctx->secret));
        data = test.c_str();
    }
    else if (test.find("from_tag") != std::string::npos)
    {
        test = fmt::format(test, fmt::arg("from_tag", upload_ctx->from.c_str()));
        data = test.c_str();
    }
    if (data)
    {
        const size_t len = strlen(data);
        memcpy(ptr, data, len);
        upload_ctx->lines_read++;
        return len;
    }
    return 0;
}
size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp)
{
   return size * nmemb;
}

Mailer::Mailer(std::string from, std::string login, std::string password)
    : m_from("<" + std::move(from) + ">")
    , m_login(std::move(login))
    , m_password(std::move(password))
{}

CT::ask_token_response_result Mailer::SendToken(const std::string& to_email)
{
    auto token = get_random_string(5, random_string_type::numeric);
    PLOG_INFO << "to_email=" << to_email << ", token=" << token;
    if (!SendTokenImpl(to_email, token))
        return CT::ask_token_response_result_MAIL_WAS_NOT_SENT;
    std::lock_guard<decltype(m_mtx)> lock {m_mtx};
    m_tokens.insert_or_assign(to_email, std::move(token));
    return CT::ask_token_response_result_OK;
}

CT::verify_token_response_result Mailer::VerifyToken(const std::string& email, const std::string& token)
{
    std::lock_guard<decltype(m_mtx)> lock {m_mtx};
    const auto it = m_tokens.find(std::string(email));
    PLOG_INFO_IF(it == std::end(m_tokens)) << "token=" << token << ", for email=" << email << " not found";
    if (it != std::end(m_tokens))
    {
        if (it->second == token)
            return CT::verify_token_response_result_OK;
        return CT::verify_token_response_result_WRONG_TOKEN;
    }
    return CT::verify_token_response_result_EMAIL_NOT_FOUND;
}

bool Mailer::SendTokenImpl(std::string to, std::string token)
{
    curl_slist *recipients = nullptr;
    upload_status upload_ctx;
    to = "<" + to + ">";
    upload_ctx.lines_read = 0;
    upload_ctx.from = m_from;
    upload_ctx.to = to;
    upload_ctx.secret = std::move(token);
 
    CURL* curl = curl_easy_init();
    PLOG_ERROR_IF(!curl) << "curl == nullptr";
    if (curl)
    {
        /* This is the URL for your mailserver */
        curl_easy_setopt(curl, CURLOPT_USERNAME, m_login.c_str());
        curl_easy_setopt(curl, CURLOPT_PASSWORD, m_password.c_str());
        curl_easy_setopt(curl, CURLOPT_URL, "smtp://smtp.gmail.com:587");
        curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        curl_easy_setopt(curl, CURLOPT_MAIL_FROM, m_from.c_str());

        recipients = curl_slist_append(recipients, to.c_str());
        curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);

        /* We're using a callback function to specify the payload (the headers and
         * body of the message). You could just use the CURLOPT_READDATA option to
         * specify a FILE pointer to read from. */ 
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);
        curl_easy_setopt(curl, CURLOPT_READDATA, &upload_ctx);
        curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);

        /* Send the message */
        const CURLcode res = curl_easy_perform(curl);

        /* Check for errors */ 
        if(res != CURLE_OK)
        {
            PLOG_ERROR << "mail was not send, error=" << curl_easy_strerror(res);
            return false;
        }
          
        /* Free the list of recipients */ 
        curl_slist_free_all(recipients);
        curl_easy_cleanup(curl);
        return true;
    }
    return false;
}
