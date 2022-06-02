#include "inc/database/DBMongo.h"
#include "come_together.pb.h"

#include <boost/property_tree/ptree.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/builder/stream/array.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/helpers.hpp>
#include <jwt-cpp/jwt.h>

#include <mongocxx/uri.hpp>
#include <plog/Log.h>

#include "utils/catch_all.h"

using bsoncxx::builder::stream::close_array;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::open_document;
const auto db_name = "come_together";
const auto users_collection_name = "users";
const auto markers_collection_name = "markers";
const auto chats_collection_name = "chats";

namespace CT = come_together_grpc;

DBMongo::DBMongo(const boost::property_tree::ptree& config)
    : m_pool(mongocxx::uri(config.get<std::string>("mongo.connection_string", "mongodb://localhost:27017")))
{
    PLOG_INFO << "connected to MongoDB";
}

CT::check_response::result DBMongo::Check(const CT::check_request& req)
{
    return {};
}

CT::register_response::result DBMongo::RegisterUser(const CT::register_request& req, std::string& out_uuid)
{
    return {};
}

CT::login_response::result DBMongo::LoginUser(const CT::login_request& req, std::string& out_uuid,
                                              std::string& out_access_token) try
{
    if (req.typ() != CT::login_request_type_BY_GOOGLE_TOKEN)
    {
        PLOG_ERROR << "unsupported request auth type for MongoDB, request: " << req.Utf8DebugString();
        return CT::login_response_result_OTHER;
    }
    const auto entry = m_pool.acquire();
    auto users_collection = (*entry)[db_name][users_collection_name];
    //std::string token {"eyJhbGciOiJSUzI1NiIsImtpZCI6IjQ4NmYxNjQ4MjAwNWEyY2RhZjI2ZDkyMTQwMThkMDI5Y2E0NmZiNTYiLCJ0eXAiOiJKV1QifQ.eyJpc3MiOiJodHRwczovL2FjY291bnRzLmdvb2dsZS5jb20iLCJhenAiOiI2Mjc4MDY3NDMyNTMtdXM5NGQ1dnE0N2F2bmE2ZTRvdTJsYWIyNXNiamNhbXUuYXBwcy5nb29nbGV1c2VyY29udGVudC5jb20iLCJhdWQiOiI2Mjc4MDY3NDMyNTMtdXM5NGQ1dnE0N2F2bmE2ZTRvdTJsYWIyNXNiamNhbXUuYXBwcy5nb29nbGV1c2VyY29udGVudC5jb20iLCJzdWIiOiIxMTQ0MDc0MDcwNzMwNTQzMzc4MDAiLCJoZCI6Im51cmUudWEiLCJlbWFpbCI6ImFsaXNhLmxldmFkbmFAbnVyZS51YSIsImVtYWlsX3ZlcmlmaWVkIjp0cnVlLCJhdF9oYXNoIjoiNmI2eGladUdJLUpaWTQ0cFFBMXNTUSIsIm5vbmNlIjoiQ0kxN3dFRmt6QXVleU44ZWx1S1ZVVkp6aXFuYUZVVDduMmdJM3VvYzZ1WSIsIm5hbWUiOiLQkNC70ZbRgdCwINCb0LXQstCw0LTQvdCwIiwicGljdHVyZSI6Imh0dHBzOi8vbGgzLmdvb2dsZXVzZXJjb250ZW50LmNvbS9hLS9BT2gxNEdqaU02VmJ5SmRfR0lwRi05QVo1aWdjWlp4VXpMRW9ycTVDSXliNT1zOTYtYyIsImdpdmVuX25hbWUiOiLQkNC70ZbRgdCwIiwiZmFtaWx5X25hbWUiOiLQm9C10LLQsNC00L3QsCIsImxvY2FsZSI6InJ1IiwiaWF0IjoxNjUzMjI0NDE5LCJleHAiOjE2NTMyMjgwMTl9.hcaJt1FVqz1NGslyjICwFfC7-9EQZpJ4es8_Bmw5EVEcRJ6QKnFBbv8D4BfaaSnSBbMoHTGNUfskffVreWB7iol9j537X99AUSMNYXFDETta6SPTDb3IX56iSkQF5gLWfHWUHGi4V7zeYV8AvcAhyxWzUb0z_swrhA9w3mpBUQKykfR9x4WfCOGR5u1qLLzGC58q_6LVBlHSeNjld7wwbfl5nUzOOXB_y5ziWVdoL2_e2VUSTLPy9kJLvArcy4DUvPOcj77zh4FWaWCapVMuSMm4au1_OYjqZX_G8Xkf__syddCJ8A6y8tXaRSEoGvMPDlOWe-DmT4e6vOB-cn5L4A"};
    auto decoded = jwt::decode(req.token().value());
    decltype(auto) payload_claims = decoded.get_payload_claims();

    const auto& email = payload_claims["email"].as_string();
    out_uuid = email;
    const auto maybe_result = users_collection.find_one(document{} << "email" << email << finalize);
    if (maybe_result)
    {
        users_collection.update_one(document{} << "email" << email << finalize,
                                    document{} << "$set" << open_document
                                    << "email" << email
                                    << "family_name" << decoded.get_payload_claims()["family_name"].as_string()
                                    << "given_name" << decoded.get_payload_claims()["given_name"].as_string()
                                    << "picture" << decoded.get_payload_claims()["picture"].as_string()
                                    << "name" << decoded.get_payload_claims()["name"].as_string()
                                    << close_document << finalize);
    }
    else
    {
        const auto res = document{} << "email" << email
            << "family_name" << decoded.get_payload_claims()["family_name"].as_string()
            << "given_name" << decoded.get_payload_claims()["given_name"].as_string()
            << "picture" << decoded.get_payload_claims()["picture"].as_string()
            << "name" << decoded.get_payload_claims()["name"].as_string()
            << finalize;
        users_collection.insert_one(res.view());
    }
    return CT::login_response_result_OK;
}
CATCH_ALL(CT::login_response_result_OTHER)

CT::user_info DBMongo::GetUserInfo(const std::string& user_uuid)
{
    return {};
}

std::vector<std::string> DBMongo::GetAllImagesUuid(const CT::get_images_request& req)
{
    return {};
}

std::array<std::string, 2> DBMongo::AddMarker(const CT::marker_info& info)
{
    const auto entry = m_pool.acquire();
    auto markers_collection = (*entry)[db_name][markers_collection_name];
    auto chats_collection = (*entry)[db_name][chats_collection_name];
    const auto marker = document{} << "category" << info.cat()
        << "subcategory" << info.sub_cat()
        << "marker_type" << info.marker_type()
        << "from_unix_time" << info.from_unix_time()
        << "to_unix_time" << info.to_unix_time()
        << "creation_unix_time" << info.creation_unix_time()
        << "creator_uuid" << info.creator_uuid()
        << "display_name" << info.display_name()
        << "latitude" << info.latitude()
        << "longitude" << info.longitude()
        << "other_data_json" << info.other_data_json()
        << finalize;
    const auto res = markers_collection.insert_one(marker.view());
    if (!res)
    {
        PLOG_ERROR << "unable to insert marker";
        return {};
    }
    const auto marker_id = res->inserted_id().get_int64();
    const auto chat = document{} << "marker_id" << marker_id
        << "parts" << open_array
        << info.creator_uuid() << close_array
        << finalize;
    const auto chat_res = chats_collection.insert_one(chat.view());
    if (!chat_res)
    {
        PLOG_ERROR << "unable to insert chat";
        return {};
    }
    return {std::to_string(marker_id), std::to_string(chat_res->inserted_id().get_int64())};
}

std::vector<CT::marker_info> DBMongo::GetAllMarkers()
{
    return {};
}

std::multimap<std::string, std::string> DBMongo::GetAllPushTokens()
{
    return {};
}
