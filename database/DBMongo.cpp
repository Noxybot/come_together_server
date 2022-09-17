#include "inc/database/DBMongo.h"
#include "core/UserPresenceInterface.h"
#include "come_together.pb.h"

#include <boost/property_tree/ptree.hpp>
#include <bsoncxx/json.hpp>
#include <bsoncxx/builder/stream/array.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/helpers.hpp>
#include <jwt-cpp/jwt.h>

#include <mongocxx/uri.hpp>
#include <mongocxx/client.hpp>
#include <plog/Log.h>

#include "core/UserStorageInterface.h"
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

DBMongo::DBMongo(const boost::property_tree::ptree& config, std::shared_ptr<UserStorageInterface> storage)
    : m_pool(mongocxx::uri(config.get<std::string>("mongo.connection_string", "mongodb://localhost:27017")))
	, m_users(std::move(storage))
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
                                    << "first_name" << decoded.get_payload_claims()["family_name"].as_string()
                                    << "last_name" << decoded.get_payload_claims()["given_name"].as_string()
                                    << "picture_url" << decoded.get_payload_claims()["picture"].as_string()
                                    << "name" << decoded.get_payload_claims()["name"].as_string()
                                    << close_document
                                    << "$addToSet" << open_document << "app_ids" << req.app_id().id() << close_document
                                    << finalize);
    }
    else
    {
        const auto res = document{} << "email" << email
            << "first_name" << decoded.get_payload_claims()["family_name"].as_string()
            << "last_name" << decoded.get_payload_claims()["given_name"].as_string()
            << "picture_url" << decoded.get_payload_claims()["picture"].as_string()
            << "name" << decoded.get_payload_claims()["name"].as_string()
            << "app_ids" << open_array << req.app_id().id() << close_array
            << finalize;
        users_collection.insert_one(res.view());
    }
    return CT::login_response_result_OK;
}
CATCH_ALL(CT::login_response_result_OTHER)

template <class T = std::string>
T GetValue(bsoncxx::document::view view, const std::string& v_name)
{
	auto it = view.find(v_name);
    if (it == std::end(view))
        return {};
    if constexpr (std::is_same_v<T, std::string>)
        return std::string(it->get_utf8().value);
    if constexpr (std::is_same_v<T, std::int32_t>)
        return it->get_int32().value;
    if constexpr (std::is_same_v<T, std::int64_t>)
        return it->get_int64().value;
    return {};
}

CT::user_info DBMongo::GetUserInfo(const std::string& user_uuid)
{
    const auto entry = m_pool.acquire();
    auto users_collection = (*entry)[db_name][users_collection_name];
    auto maybe_result =
        users_collection.find_one(document{} << "email" << user_uuid << finalize);
    CT::user_info u_info;
    if (!maybe_result)
        return u_info;
    const auto view = maybe_result.get().view();
	u_info.set_other_info_json(GetValue(view, "other_info"));
	u_info.set_first_name(GetValue(view, "first_name"));
	u_info.set_last_name(GetValue(view, "last_name"));
	u_info.set_country(GetValue<std::string>(view, "country"));
	u_info.set_city(GetValue<std::string>(view, "city"));
	u_info.set_bio(GetValue<std::string>(view, "bio"));
	u_info.set_unix_time_of_birth(GetValue<std::int64_t>(view, "date_of_birth"));
	u_info.set_other_info_json(GetValue<std::string>(view, "other_info"));
	u_info.set_verif_result(
        static_cast<CT::verification_result>(GetValue<std::int32_t>(view, "verification_result")));
    auto url = GetValue<std::string>(view, "picture_url");
    if (!url.empty())
    {
        CT::image img;
        img.set_typ(CT::image_type_URL);
        img.set_text(std::move(url));
        u_info.mutable_images()->Add(std::move(img));
    }
    auto img_uuids = view.find("image_ids");
    if (img_uuids == std::end(view))
        return u_info;
    for (const auto& id : img_uuids->get_array().value)
    {
        CT::image img;
        img.set_typ(CT::image_type_UUID);
        img.set_uuid(std::string(id.get_utf8().value));
        u_info.mutable_images()->Add(std::move(img));
    }
    return u_info;
}

std::vector<std::string> DBMongo::GetAllImagesUuid(const CT::get_images_request& req)
{
    std::vector<std::string> res;

    if (req.image_type() != CT::get_images_request_type_USER && req.image_type() != CT::get_images_request_type_MARKER)
    {
        PLOG_ERROR << "unknown image type: " << req.image_type();
        return res;
    }
    const auto entry = m_pool.acquire();
    auto collection = (*entry)[db_name]
         [req.image_type() == CT::get_images_request_type_USER ? users_collection_name : markers_collection_name];
    auto maybe_result =
        collection.find_one(document{} << "_id" << bsoncxx::oid(req.target_uuid()) << finalize);
    if (!maybe_result)
    {
        PLOG_INFO << "images for " << req.target_uuid() << " not found";
        return res;
    }
    const auto view = maybe_result.get().view();
	auto arr_it = view.find("image_ids");
    if (arr_it == std::end(view))
    {
        PLOG_INFO << "images for " << req.target_uuid() << " not found";
        return res;
    }
    for (const auto& image : arr_it->get_array().value)
        res.emplace_back(image.get_utf8().value);
    return res;
}

std::array<std::string, 2> DBMongo::AddMarker(const CT::marker_info& info) try
{
    if (info.creator_uuid().empty())
    {
        PLOG_ERROR << "can`t create marker with empty creator";
        return {};
    }
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
        << "expenses_from" << info.expenses_from()
        << "expenses_to" << info.expenses_to()
        << "people_from" << info.people_from()
        << "people_to" << info.people_to()
        << "description" << info.description()
        << "address" << info.address()
        << "display_name" << info.display_name()
        << "latitude" << info.latitude()
        << "longitude" << info.longitude()
        //<< "image" << img
        << "other_data_json" << bsoncxx::from_json(info.other_data_json().empty() ? "{}" : info.other_data_json())
        << finalize;
    const auto res = markers_collection.insert_one(marker.view());
    if (!res)
    {
        PLOG_ERROR << "unable to insert marker";
        return {};
    }
    const auto marker_id = res->inserted_id().get_oid().value.to_string();
    PLOG_INFO << "add marker with id " << marker_id;
    const auto chat = document{} << "marker_id" << marker_id << "display_name" << info.display_name()
        << "parts" << open_array
        << info.creator_uuid() << close_array
        << finalize;
    const auto chat_res = chats_collection.insert_one(chat.view());
    if (!chat_res)
    {
        PLOG_ERROR << "unable to insert chat";
        return {};
    }
    const auto chat_id = chat_res->inserted_id().get_oid().value.to_string();
    markers_collection.update_one(document{} << "_id" << bsoncxx::oid(marker_id) << finalize,
        document{} << "$set" << open_document
        << "chat_id" << chat_id
        << close_document << finalize);
    return {std::string(marker_id), chat_id };
}
CATCH_ALL({})

std::vector<CT::marker_info> DBMongo::GetAllMarkers() try
{
    const auto entry = m_pool.acquire();
    auto markers_collection = (*entry)[db_name][markers_collection_name];
    auto chats_collection = (*entry)[db_name][chats_collection_name];
    auto maybe_result =
        markers_collection.find({});
    std::vector<CT::marker_info> res;
    for (auto& marker : maybe_result) {
        CT::marker_info info;
        info.set_sub_cat(CT::marker_info_sub_category(GetValue<std::int32_t>(marker, "subcategory")));
        info.set_marker_type(CT::marker_info_type(GetValue<std::int32_t>(marker, "marker_type")));
        info.set_from_unix_time(GetValue<std::int64_t>(marker, "from_unix_time"));
        info.set_to_unix_time(GetValue<std::int64_t>(marker, "to_unix_time"));
        info.set_creation_unix_time(GetValue<std::int64_t>(marker, "creation_unix_time"));
        info.set_expenses_from(GetValue<std::int32_t>(marker, "expenses_from"));
        info.set_expenses_to(GetValue<std::int32_t>(marker, "expenses_to"));
        info.set_people_from(GetValue<std::int32_t>(marker, "people_from"));
        info.set_people_to(GetValue<std::int32_t>(marker, "people_to"));
        info.set_creator_uuid(GetValue(marker, "creator_uuid"));
        info.set_display_name(GetValue(marker, "display_name"));
        info.set_address(GetValue(marker, "address"));
        info.set_description(GetValue(marker, "description"));
        info.set_longitude(marker["longitude"].get_double().value);
        info.set_latitude(marker["latitude"].get_double().value);
        info.set_other_data_json(bsoncxx::to_json(marker["other_data_json"].get_document().value));
        const auto chat_id= GetValue(marker, "chat_id");
        info.set_chat_uuid(std::string(chat_id));
        const auto chat = chats_collection.find_one(document{} << "_id" << bsoncxx::oid(chat_id) << finalize);
        if (chat)
        {
            CT::chat_info chat_info;
            const auto view = chat->view();
            for (const auto& part : view["parts"].get_array().value)
                chat_info.add_participants_uuid((std::string(part.get_utf8().value)));
            *info.mutable_c_info() = std::move(chat_info);
        }

        //images
        auto image_ids_it = marker.find("image_ids");
        if (image_ids_it != std::end(marker))
        {
            const auto& arr = image_ids_it->get_array().value;
            for (const auto& img_id : arr)
            {
                CT::image img;
                img.set_typ(CT::image_type_UUID);
                img.set_text(std::string(img_id.get_utf8().value));
                PLOG_DEBUG << "adding img id " << img_id.get_utf8().value << " to marker";
                info.mutable_images()->Add(std::move(img));
            }
        }
        res.push_back(std::move(info));
    }
    return res;
}
CATCH_ALL({})

void DBMongo::UpdateVerifStatus(const std::string& email, CT::verification_result res)
{
    PLOG_INFO << "update user verification status " << email << ", res " << res;
    const auto entry = m_pool.acquire();
	auto users_collection = (*entry)[db_name][users_collection_name];
    users_collection.update_one(document{} << "email" << email << finalize,
        document{} << "$set" << open_document
        << "verification_result" << res
        << close_document << finalize);
}

std::multimap<std::string, std::string> DBMongo::GetAllPushTokens()
{
    return {};
}

CT::update_info_response::result DBMongo::UpdateInfo(const CT::update_info_request& req) try
{
    const auto entry = m_pool.acquire();
    const auto typ = req.type();
    if (typ == CT::update_info_request_update_type_USER)
    {
        auto users_collection = (*entry)[db_name][users_collection_name];
        const auto& u_info = req.u_info();
        const auto uuid = m_users->GetUuidByAccessToken(req.token().value());
        if (uuid.empty())
        {
            PLOG_ERROR << "user with access token: " << req.token().value() << " not found";
            return CT::update_info_response_result_OTHER;
        }
        PLOG_INFO<< "trying to update info for user: " << uuid << ", with token: " << req.token().value()
        << ", other_info: " << u_info.other_info_json();
        users_collection.update_one(document{} << "email" << uuid << finalize,
            document{} << "$set" << open_document
            << "other_info" << bsoncxx::from_json(u_info.other_info_json())
            << "first_name" << u_info.first_name()
            << "last_name" << u_info.last_name()
            << "date_of_birth" << u_info.unix_time_of_birth()
            << "bio" << u_info.bio()
            << "country" << u_info.country()
            << "city" << u_info.city()
            << close_document << finalize);
    }
    return CT::update_info_response_result_OK;

}
CATCH_ALL(CT::update_info_response_result_OTHER)

std::string DBMongo::GetUuidByAppId(const std::string& app_id) try
{
    const auto entry = m_pool.acquire();
    auto users_collection = (*entry)[db_name][users_collection_name];
    const auto res = users_collection.find_one(document{} << "app_ids" << app_id << finalize);
    if (!res)
    {
        PLOG_INFO << "uuid for app id " << app_id << " not found";
        return {};
    }
    const auto& doc_view = res.get().view();
	auto it = doc_view.find("email");
    if (it != std::end(doc_view))
    {
        std::string val(it->get_utf8().value);
        PLOG_INFO << "found email " << val << " for app id: " << app_id;
        return val;
    }
    return {};
}
CATCH_ALL({})

bool DBMongo::SaveChatMessage(const CT::chat_message& msg) try
{
    const auto entry = m_pool.acquire();
    auto chats = (*entry)[db_name][chats_collection_name];
    const auto res = chats.update_one(document{} << "_id" << bsoncxx::oid(msg.chat_uuid()) << finalize,
        document{} << "$push" << open_document
        << "messages" << open_document << "sender" << msg.sender_uuid() << "content" << msg.text()
        << "sent_time" << msg.sent_unix_time() << close_document
        << close_document << finalize);
    return res.has_value();
}
CATCH_ALL({})

std::vector<CT::chat_message> DBMongo::GetChatMessages(const std::string& chat_id) try
{
    std::vector<CT::chat_message> res;
    const auto entry = m_pool.acquire();
    auto chats = (*entry)[db_name][chats_collection_name];
	auto chat = chats.find_one(document{} << "_id" << bsoncxx::oid(chat_id) << finalize);
    if (!chat)
    {
        PLOG_INFO << "chat " << chat_id << "not found";
        return res;
    }
    const auto chat_view = chat->view();
	auto msgs = chat_view.find("messages");
    if (msgs == chat_view.end())
        return res;
    for (const auto& msg : msgs->get_array().value)
    {
        CT::chat_message msg_;
        msg_.set_sender_uuid(std::string(msg["sender"].get_utf8().value));
        msg_.set_text(std::string(msg["content"].get_utf8().value));
        msg_.set_sent_unix_time(msg["sent_time"].get_int64().value);
        res.push_back(std::move(msg_));
    }
    return res;
}
CATCH_ALL({})

std::vector<std::string> DBMongo::GetChatParts(const std::string& chat_id)
{
    const auto entry = m_pool.acquire();
    auto chats = (*entry)[db_name][chats_collection_name];
    auto chat = chats.find_one(document{} << "_id" << bsoncxx::oid(chat_id) << finalize);
    std::vector<std::string> res;
    if (!chat)
        return res;
    const auto chat_view = chat->view();
    auto parts = chat_view.find("parts");
    if (parts == chat_view.end())
        return res;
    for (const auto& part : parts->get_array().value)
        res.emplace_back(std::string(part.get_utf8().value));
    return res;
}

bool DBMongo::AddUserToChat(const std::string& chat_id, const std::string& user_id)
{
    const auto entry = m_pool.acquire();
    auto chats = (*entry)[db_name][chats_collection_name];
    PLOG_INFO << "adding user " << user_id << " to chat " << chat_id;
    const auto res = chats.update_one(document{} << "_id" << bsoncxx::oid(chat_id) << finalize,
        document{}
        << "$addToSet" << open_document << "parts" << user_id << close_document
        << finalize);
    return res.has_value();
}

CT::marker_info DBMongo::GetMarkerInfo(const std::string& marker_uuid)
{
    const auto entry = m_pool.acquire();
    auto markers = (*entry)[db_name][markers_collection_name];
    auto marker = markers.find_one(document{} << "_id" << bsoncxx::oid(marker_uuid) << finalize);
    if (!marker)
    {
        PLOG_INFO << "marker " << marker_uuid << " not found";
        return {};
    }
    CT::marker_info res;
    const auto view = marker->view();
    return {};
}

std::vector<CT::chat_info> DBMongo::GetUserChats(const std::string& user_id) try
{
    const auto entry = m_pool.acquire();
    auto chats = (*entry)[db_name][chats_collection_name];
	auto u_chats = chats.find(document{} << "parts" << user_id << finalize);
    std::vector<CT::chat_info> res;
    for (const auto& chat : u_chats)
    {
        CT::chat_info info;
        info.set_uuid(std::string(chat["_id"].get_oid().value.to_string()));
        info.set_title(std::string(chat["display_name"].get_utf8().value));
        info.set_marker_uuid(std::string(chat["marker_id"].get_utf8().value));
        for (const auto& part : chat["parts"].get_array().value)
            info.add_participants_uuid(std::string(part.get_utf8().value));
        res.push_back(std::move(info));
    }
    return res;
}
CATCH_ALL({})

bool DBMongo::AddMarkerImage(const std::string& marker_uuid, const std::string& image_uuid) try
{
    const auto entry = m_pool.acquire();
    auto markers = (*entry)[db_name][markers_collection_name];
    return markers.update_one(
        document{} << "_id" << bsoncxx::oid(marker_uuid) << finalize,
        document{} << "$addToSet" << open_document << "image_ids" << image_uuid << close_document << finalize).has_value();
}
CATCH_ALL({})
bool DBMongo::AddUserImage(const std::string& user_uuid, const std::string& image_uuid) try
{
    const auto entry = m_pool.acquire();
    auto users = (*entry)[db_name][users_collection_name];
    return users.update_one(
        document{} << "email" << user_uuid << finalize,
        document{} << "$addToSet" << open_document << "image_ids" << image_uuid << close_document << finalize).has_value();
}
CATCH_ALL({})
