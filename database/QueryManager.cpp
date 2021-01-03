#include "QueryManager.h"

#define DB_NAME "dcfcatkkl391gn"

static constexpr std::string_view CHECK_EMAIL =
"SELECT COUNT(*) FROM users WHERE email = {email}";
static constexpr std::string_view CHECK_LOGIN =
"SELECT COUNT(*) FROM users WHERE login = {login}";
static constexpr std::string_view REGISTER_USER =
"SELECT add_user({email}, {login}, {password_md5},\
{first_name}, {last_name}, {other_info})";
static constexpr std::string_view LOGIN_USER =
"SELECT login_user({login}, {password_md5})";
static constexpr std::string_view GET_USER_INFO =
"SELECT * FROM users WHERE uuid = uuid({uuid})";
static constexpr std::string_view GET_ALL_USER_IMAGES =
"SELECT images_uuid FROM users WHERE uuid = uuid({uuid})";
static constexpr std::string_view GET_ALL_MARKER_IMAGES =
"SELECT images_uuid FROM markers WHERE uuid = uuid({uuid})";

std::string_view QueryManager::Get(Query type)
{
    switch (type)
    {
    case Query::CHECK_EMAIL: return CHECK_EMAIL;
    case Query::CHECK_LOGIN: return CHECK_LOGIN;
    case Query::REGISTER_USER: return REGISTER_USER;
    case Query::LOGIN_USER: return LOGIN_USER;
    case Query::GET_USER_INFO: return GET_USER_INFO;
    case Query::GET_ALL_USER_IMAGES: return GET_ALL_USER_IMAGES;
    case Query::GET_ALL_MARKER_IMAGES: return GET_ALL_MARKER_IMAGES;
    case Query::QUERY_COUNT: break;
    default: break;
    }
    return "";
}

#undef DB_NAME