#include "inc/database/DBPostgres.h"
#include "inc/database/QueryManager.h"
#include "inc/database/ConnectionPool.h"
#include "utils/catch_all.h"
#include "utils/invoke.h"

#include <fmt/format.h>
#include <plog/Log.h>

#include <array>

std::vector<std::string> parse_array(pqxx::array_parser parser)
{
    std::vector<std::string> results;
    auto elem = parser.get_next();
    while (elem.first != pqxx::array_parser::done)
    {
        if (elem.first == pqxx::array_parser::string_value)
            results.push_back(std::move(elem.second));
        elem = parser.get_next();
    }
    return results;
}

DBPostgres::DBPostgres(const std::string& connection_string)
    : m_pool(std::make_shared<ConnectionPool>(connection_string, 10))
{}

// ReSharper disable once CppNotAllPathsReturnValue
CT::check_response::result DBPostgres::Check(const CT::check_request& req) try
{
    const auto conn = m_pool->AcquireConnection();
    pqxx::work work(*conn);
    std::string query;
    switch (req.check_type())
    {
    case CT::check_request_type_EMAIL:
        query = fmt::format(QueryManager::Get(Query::CHECK_EMAIL),
            fmt::arg("email", work.quote(req.data())));
        break;
    case CT::check_request_type_LOGIN:
        query = fmt::format(QueryManager::Get(Query::CHECK_LOGIN),
            fmt::arg("login", work.quote(req.data())));
        break;
    default:
        assert(false);
        return CT::check_response_result_OTHER;
    }
    const auto result = work.exec1(query);
    work.commit();
    switch(result[0].as<int>())
    {
    case 0:
        return CT::check_response_result_AVAILABLE;
    default:
        return CT::check_response_result_TAKEN;
    }
}
CATCH_ALL(CT::check_response_result_OTHER);

CT::register_response::result DBPostgres::RegisterUser(const CT::register_request& req, std::string& out_uuid) try
{
    const auto& user_info = req.info();
    if (user_info.email().empty() || user_info.login().empty() ||
        user_info.password().empty() || user_info.first_name().empty())
        return CT::register_response_result_NOT_SET;
    const auto conn = m_pool->AcquireConnection();
    pqxx::work work(*conn);
    const std::string query =
        fmt::format(QueryManager::Get(Query::REGISTER_USER),
        fmt::arg("email", work.quote(user_info.email())),
        fmt::arg("login", work.quote(user_info.login())),
        fmt::arg("password_md5", work.quote(user_info.password())), //todo: add hashing
        fmt::arg("first_name", work.quote(user_info.first_name())),
        fmt::arg("last_name", work.quote(user_info.last_name())),
        fmt::arg("other_info",
            work.quote(user_info.other_info_json().empty() ? "{}" : user_info.other_info_json())));
    const auto res = work.exec1(query);
    work.commit();
    auto uuid = res[0].as<std::string>();
    if (uuid == "1")
        return CT::register_response_result_EMAIL_ALREADY_TAKEN;
    if (uuid == "2")
        return CT::register_response_result_LOGIN_ALREADY_TAKEN;
    PLOG_ERROR_IF(uuid.empty()) << "empty uuid from db";
    out_uuid = std::move(uuid);
    return CT::register_response_result_OK;
}
CATCH_ALL(CT::register_response_result_OTHER)

CT::login_response::result DBPostgres::LoginUser(const CT::login_request& req, std::string& out_uuid, std::string& out_access_token) try
{
    const auto conn = m_pool->AcquireConnection();
    pqxx::work work(*conn);
    std::string query;
    switch (req.typ())
    {
    case CT::login_request_type_BY_LOGIN_PASSWORD:
        query =
            fmt::format(QueryManager::Get(Query::LOGIN_USER_BY_PASS),
            fmt::arg("login", work.quote(req.login())),
            fmt::arg("password_md5", work.quote(req.password())),
            fmt::arg("app_id", work.quote(req.app_id().id())));
        break;
    case CT::login_request_type_BY_ACCESS_TOKEN:
        query =
            fmt::format(QueryManager::Get(Query::LOGIN_USER_BY_TOKEN),
            fmt::arg("access_token", work.quote(req.token().value())));
        break;
    default:
        PLOG_ERROR << "Unsupported login type";
        return CT::login_response::result::login_response_result_OTHER;
    }
    const auto res = work.exec1(query);
    work.commit();
    if (req.typ() == CT::login_request_type_BY_ACCESS_TOKEN)
    {
        auto val = res[0].as<std::string>();
        if (val == "1")
        {
            PLOG_INFO << "access token not found";
            return CT::login_response::result::login_response_result_ACCESS_TOKEN_NOT_FOUND;
        }
        out_uuid = std::move(val);
        out_access_token = req.token().value();
        return CT::login_response_result_OK;
    }
    std::vector<std::string> results = parse_array(res[0].as_array());
    if (results.empty())
    {
        PLOG_ERROR << "db schema changed";
        return CT::login_response_result_OTHER;
    }
    auto& uuid = results[0];
    if (uuid == "1")
        return CT::login_response_result_USER_NOT_FOUND;
    if (uuid == "2")
        return CT::login_response_result_WRONG_PASSWORD;
    PLOG_ERROR_IF(uuid.empty()) << "empty uuid from db";
    out_uuid = std::move(uuid);
    if (results.size() == 2)
        out_access_token = std::move(results[1]);
    else
        PLOG_ERROR << "db schema changed";
    return CT::login_response_result_OK;
}
CATCH_ALL(CT::login_response_result_OTHER)

// ReSharper disable once CppNotAllPathsReturnValue
CT::user_info DBPostgres::GetUserInfo(const std::string& user_uuid) try
{
    const auto conn = m_pool->AcquireConnection();
    pqxx::work work(*conn);
    const auto query =
        fmt::format(QueryManager::Get(Query::GET_USER_INFO),
        fmt::arg("uuid", work.quote(user_uuid)));
    const auto res = work.exec(query);
    work.commit();
    if (res.empty())
        return {};
    PLOG_ERROR_IF(res.size() != 1) << "found more than one user";
    CT::user_info info;
    const auto& row = res[0];
    PLOG_ERROR_IF(row.size() != 8) << "db schema changed";
    invoker(info, row,
        cast_member(&CT::user_info::set_uuid),
        cast_member(&CT::user_info::set_email),
        cast_member(&CT::user_info::set_login),
        cast_member(&CT::user_info::set_password),
        cast_member(&CT::user_info::set_first_name),
        cast_member(&CT::user_info::set_last_name));
    auto parser = row[6].as_array(); //image uuids
    auto elem = parser.get_next();
    while (elem.first != pqxx::array_parser::done)
    {
        if (elem.first == pqxx::array_parser::string_value)
            info.mutable_images_uuid()->Add(std::move(elem.second));
        elem = parser.get_next();
    }
    info.set_other_info_json(row[7].as<std::string>());
    return info;
}
CATCH_ALL({})

std::vector<std::string> DBPostgres::GetAllImagesUuid(const CT::get_images_request& req)
{
    const auto conn = m_pool->AcquireConnection();
    pqxx::work work(*conn);
    std::string query;
    std::vector<std::string> result;
    switch (req.image_type())
    {
    case CT::get_images_request_type_USER:
        query = fmt::format(QueryManager::Get(Query::GET_ALL_USER_IMAGES),
                fmt::arg("uuid", work.quote(req.target_uuid())));
        break;
    case CT::get_images_request_type_MARKER:
        query = fmt::format(QueryManager::Get(Query::GET_ALL_MARKER_IMAGES),
                fmt::arg("uuid", work.quote(req.target_uuid())));
        break;
    default:
        PLOG_WARNING << "unknown type";
        return result;
    }
    const auto res = work.exec1(query);
    work.commit();
    auto parser = res[0].as_array();
    auto elem = parser.get_next();
    while (elem.first != pqxx::array_parser::done)
    {
        if (elem.first == pqxx::array_parser::string_value)
            result.push_back(std::move(elem.second));
        elem = parser.get_next();
    }
    return result;
}

std::array<std::string, 2> DBPostgres::AddMarker(const CT::marker_info& info)try
{
    std::array<std::string, 2> res;
    const auto conn = m_pool->AcquireConnection();
    pqxx::work work(*conn);
    const auto query =
        fmt::format(QueryManager::Get(Query::ADD_MARKER),
            fmt::arg("category", info.cat()),
            fmt::arg("type", info.marker_type()),
            fmt::arg("from_unix_time", info.from_unix_time()),
            fmt::arg("to_unix_time", info.to_unix_time()),
            fmt::arg("creation_unix_time", info.creation_unix_time()),
            fmt::arg("creator_uuid", work.quote(info.creator_uuid())),
            fmt::arg("display_name", work.quote(info.display_name())),
            fmt::arg("latitude", info.latitude()),
            fmt::arg("longitude", info.longitude()),
            fmt::arg("other_data_json", work.quote(info.other_data_json())));
    const auto result = work.exec1(query);
    work.commit();
    PLOG_ERROR_IF(info.marker_type() == CT::marker_info_type_PRIVATE && result.size() != 1) << "wrong result from db";
    PLOG_ERROR_IF(info.marker_type() == CT::marker_info_type_GROUP && result.size() != 1) << "wrong result from db";
    auto parser = result[0].as_array();
    auto elem = parser.get_next();
    auto i = 0u;
    while (elem.first != pqxx::array_parser::done)
    {
        if (i >= 2)
            break;
        if (elem.first == pqxx::array_parser::string_value)
            res[i++] = (std::move(elem.second)); 
        elem = parser.get_next();
    }
    return res;
}
CATCH_ALL({})

std::vector<CT::marker_info> DBPostgres::GetAllMarkers() try
{
    std::vector<CT::marker_info> res;
    const auto conn = m_pool->AcquireConnection();
    pqxx::work work(*conn);
    const auto result = work.exec(std::string(QueryManager::Get(Query::GET_ALL_MARKERS)));
    work.commit();
    for(const auto& marker_row : result)
    {
        CT::marker_info info;
        info.set_latitude(marker_row[8].as<double>());
        info.set_longitude(marker_row[9].as<double>());
        res.push_back(std::move(info));
    }
    return res;
}
CATCH_ALL({})

std::multimap<std::string, std::string> DBPostgres::GetAllPushTokens() try
{
    std::multimap<std::string, std::string> res;
    const auto conn = m_pool->AcquireConnection();
    pqxx::work work(*conn);
    const auto result = work.exec(std::string(QueryManager::Get(Query::GET_ALL_PUSH_TOKENS)));
    work.commit();
    for (const auto& row: result)
    {
        if (row.size() == 2)
            res.emplace(row[0].as<std::string>(), row[1].as<std::string>());
        else
            PLOG_ERROR << "db schema changed";
    }
    return res;
}
CATCH_ALL({})
