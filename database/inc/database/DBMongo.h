#pragma once
#include <boost/property_tree/ptree_fwd.hpp>
#include <mongocxx/database.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/pool.hpp>
#include "DBInterface.h"

class UserStorageInterface;

class DBMongo : public DBInterface
{
    mongocxx::instance m_instance;
    mongocxx::pool m_pool;
    std::shared_ptr<UserStorageInterface> m_users;
public:
    explicit DBMongo(const boost::property_tree::ptree& config, std::shared_ptr<UserStorageInterface> storage);
    CT::check_response::result Check(const CT::check_request& req) override;
    CT::register_response::result RegisterUser(const CT::register_request& req, std::string& out_uuid) override;
    CT::login_response::result LoginUser(const CT::login_request& req, std::string& out_uuid,
                                         std::string& out_access_token) override;
    CT::user_info GetUserInfo(const std::string& user_uuid) override;
    std::vector<std::string> GetAllImagesUuid(const CT::get_images_request& req) override;
    std::array<std::string, 2> AddMarker(const CT::marker_info& info) override;
    std::vector<CT::marker_info> GetAllMarkers() override;
    void UpdateVerifStatus(const std::string& email, CT::verification_result res) override;
    std::multimap<std::string, std::string> GetAllPushTokens() override;
    CT::update_info_response::result UpdateInfo(const CT::update_info_request& req) override;
    std::string GetUuidByAppId(const std::string& app_id) override;
    bool SaveChatMessage(const CT::chat_message& msg) override;
    std::vector<CT::chat_message> GetChatMessages(const std::string& chat_id) override;
    std::vector<std::string> GetChatParts(const std::string& chat_id) override;
    bool AddUserToChat(const std::string& chat_id, const std::string& user_id) override;
    CT::marker_info GetMarkerInfo(const std::string& marker_uuid) override;
    std::vector<CT::chat_info> GetUserChats(const std::string& user_id) override;
    bool AddMarkerImage(const std::string& marker_uuid, const std::string& image_uuid) override;
    bool AddUserImage(const std::string& user_uuid, const std::string& image_uuid) override;
};
