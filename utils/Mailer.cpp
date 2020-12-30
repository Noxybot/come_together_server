#include "Mailer.h"
#include <random>
#include <algorithm>
#include <cstddef>
#include <cstring>
#include <curl/curl.h>
#include <fmt/format.h>
static const std::size_t name_size = 16;
#define FROM    "<eduard.voronkin@nure.ua>"
#define TO      "<eduard.voronkin@nure.ua>"
#define CC      "<eduard.voronkin@nure.ua>"
static std::vector<char> charset()
{
    //Change this to suit
    return std::vector<char>( 
    {'0','1','2','3','4',
    '5','6','7','8','9',
    'A','B','C','D','E','F',
    'G','H','I','J','K',
    'L','M','N','O','P',
    'Q','R','S','T','U',
    'V','W','X','Y','Z',
    'a','b','c','d','e','f',
    'g','h','i','j','k',
    'l','m','n','o','p',
    'q','r','s','t','u',
    'v','w','x','y','z'
    });
};    

// given a function that generates a random character,
// return a string of the requested length
template <class Functor>
static std::string random_string( size_t length, Functor rand_char )
{
    std::string str(length,0);
    std::generate_n(str.begin(), length, rand_char);
    return str;
}

 //0) create the character set.
    //   yes, you can use an array here, 
    //   but a function is cleaner and more flexible
const auto ch_set = charset();

//1) create a non-deterministic random number generator      
static std::default_random_engine rng(std::random_device{}());

//2) create a random number "shaper" that will give
//   us uniformly distributed indices into the character set
static std::uniform_int_distribution<> dist(0, ch_set.size()-1);

//3) create a function that ties them together, to get:
//   a non-deterministic uniform distribution from the 
//   character set of your choice.
static char randchar()
{
    return ch_set[ dist(rng) ];
}

//static const char *payload_text[] = {
//  "Date: Mon, 29 Nov 2010 21:54:29 +1100\r\n",
//  "To: <{to_tag}>" "\r\n",
//  "From: " FROM " (Birdy App)\r\n",
//  "Message-ID: <dcd7cb36-11db-487a-9f3a-e652a9658efd@"
//  "rfcpedant.example.org>\r\n",
//  "Subject: Birdy password\r\n",
//  "\r\n", /* empty line to divide headers from body, see RFC5322 */ 
//  "{secret_tag}\r\n",
//  NULL
//};

//static const char *payload_text[] = {
//  "Date: Mon, 29 Nov 2010 21:54:29 +1100\r\n",
//  "To: " TO "\r\n",
//  "From: " FROM " (Example User)\r\n",
//  "Cc: " CC " (Another example User)\r\n",
//  "Message-ID: <dcd7cb36-11db-487a-9f3a-e652a9458efd@"
//  "rfcpedant.example.org>\r\n",
//  "Subject: SMTP TLS example message\r\n",
//  "\r\n", /* empty line to divide headers from body, see RFC5322 */
//  "The body of the message starts here.\r\n",
//  "\r\n",
//  "It could be a lot of lines, could be MIME encoded, whatever.\r\n",
//  "Check RFC5322.\r\n",
//  NULL
//};
 #define FROM    "<edikshel@gmail.com>"
//#define TO      "<edikshel@gmail.com>"
#define CC      "edikshel@gmail.com>"

static const char *payload_text[] = {
  "Date: Mon, 29 Nov 2010 21:54:29 +1100\r\n",
  "To: {to_tag}" "\r\n",
  "From: " FROM " (Example User)\r\n",
  //"Cc: " CC " (Another example User)\r\n",
  "Message-ID: <dcd7cb36-11db-487a-9f3a-e652a9458efd@"
  "rfcpedant.example.org>\r\n",
  "Subject: Birdy pass {rand_tag}\r\n",
  "\r\n", /* empty line to divide headers from body, see RFC5322 */
  "Body.\r\n",
  "\r\n",
  "{secret_tag}\r\n",
  NULL
};
struct upload_status {
  int lines_read;
  std::string to, secret;
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
  else if (test.find("rand_tag") != std::string::npos)
  {
      test = fmt::format(test, fmt::arg("rand_tag", upload_ctx->secret));
      data = test.c_str();
  }
  if(data)
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
static int send_mail(std::string to_, std::string secret_)
{
  CURL *curl;
  CURLcode res = CURLE_OK;
  struct curl_slist *recipients = NULL;
  struct upload_status upload_ctx;
  to_ = "<" + to_ + ">";
  upload_ctx.lines_read = 0;
  upload_ctx.to = to_;
  upload_ctx.secret = secret_;
 
  curl = curl_easy_init();
  if(curl) {
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    //curl_easy_setopt (curl, CURLOPT_VERBOSE, 0L); //0 disable messages
    /* Set username and password */ 
    curl_easy_setopt(curl, CURLOPT_USERNAME, "edikshel@gmail.com");
    curl_easy_setopt(curl, CURLOPT_PASSWORD, "Allisfine33");
    /* This is the URL for your mailserver. Note the use of port 587 here,
     * instead of the normal SMTP port (25). Port 587 is commonly used for
     * secure mail submission (see RFC4403), but you should use whatever
     * matches your server configuration. */ 
    curl_easy_setopt(curl, CURLOPT_URL, "smtp://smtp.gmail.com:587");
 
    /* In this example, we'll start with a plain text connection, and upgrade
     * to Transport Layer Security (TLS) using the STARTTLS command. Be careful
     * of using CURLUSESSL_TRY here, because if TLS upgrade fails, the transfer
     * will continue anyway - see the security discussion in the libcurl
     * tutorial for more details. */ 
    curl_easy_setopt(curl, CURLOPT_USE_SSL, (long)CURLUSESSL_ALL);
 
    /* If your server doesn't have a valid certificate, then you can disable
     * part of the Transport Layer Security protection by setting the
     * CURLOPT_SSL_VERIFYPEER and CURLOPT_SSL_VERIFYHOST options to 0 (false).
     *   curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
     *   curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
     * That is, in general, a bad idea. It is still better than sending your
     * authentication details in plain text though.  Instead, you should get
     * the issuer certificate (or the host certificate if the certificate is
     * self-signed) and add it to the set of certificates that are known to
     * libcurl using CURLOPT_CAINFO and/or CURLOPT_CAPATH. See docs/SSLCERTS
     * for more information. */ 
    //curl_easy_setopt(curl, CURLOPT_CAINFO, "/path/to/certificate.pem");
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
      curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
 
    /* Note that this option isn't strictly required, omitting it will result
     * in libcurl sending the MAIL FROM command with empty sender data. All
     * autoresponses should have an empty reverse-path, and should be directed
     * to the address in the reverse-path which triggered them. Otherwise,
     * they could cause an endless loop. See RFC 5321 Section 4.5.5 for more
     * details.
     */ 
    curl_easy_setopt(curl, CURLOPT_MAIL_FROM, FROM);
 
    /* Add two recipients, in this particular case they correspond to the
     * To: and Cc: addressees in the header, but they could be any kind of
     * recipient. */ 
    recipients = curl_slist_append(recipients, to_.c_str());//to_.c_str());//std::string("<" + to_ + ">").c_str());
    curl_easy_setopt(curl, CURLOPT_MAIL_RCPT, recipients);
 
    /* We're using a callback function to specify the payload (the headers and
     * body of the message). You could just use the CURLOPT_READDATA option to
     * specify a FILE pointer to read from. */ 
    curl_easy_setopt(curl, CURLOPT_READFUNCTION, payload_source);
    curl_easy_setopt(curl, CURLOPT_READDATA, &upload_ctx);
    curl_easy_setopt(curl, CURLOPT_UPLOAD, 1L);
 
    /* Since the traffic will be encrypted, it is very useful to turn on debug
     * information within libcurl to see what is happening during the transfer.
     */ 
    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
 
    /* Send the message */ 
    res = curl_easy_perform(curl);
 
    /* Check for errors */ 
    if(res != CURLE_OK)
      fprintf(stderr, "curl_easy_perform() failed: %s\n",
              curl_easy_strerror(res));
 
    /* Free the list of recipients */ 
    curl_slist_free_all(recipients);
 
    /* Always cleanup */ 
    curl_easy_cleanup(curl);
  }
    return res;
}

Mailer::Mailer(const std::string& from)
{

}

void Mailer::SendToken(const std::string& to_email)
{
    auto token = random_string(name_size, randchar);
    send_mail(to_email, token);
    std::lock_guard<decltype(m_mtx)> lock {m_mtx};
    m_tokens.insert_or_assign(to_email, std::move(token));
}

CT::verify_token_response::result Mailer::VerifyToken(const std::string& email, const std::string& token)
{
    std::lock_guard<decltype(m_mtx)> lock {m_mtx};
    auto it = m_tokens.find(std::string(email));
    if (it != std::end(m_tokens))
        return it->second == token;
    return false;
}

