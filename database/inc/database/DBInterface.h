#pragma once
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "come_together.pb.h"

namespace CT = come_together_grpc;

class DBInterface
{
public:
    using Ptr = std::shared_ptr<DBInterface>;
    virtual ~DBInterface() = default;
    virtual CT::check_response::result Check(const CT::check_request& req) = 0;
    virtual CT::register_response::result RegisterUser(const CT::register_request& req, std::string& out_uuid) = 0;
    virtual CT::login_response::result LoginUser(const CT::login_request& req, std::string& out_uuid, std::string& out_access_token) = 0;
    virtual CT::user_info GetUserInfo(const std::string& user_uuid) = 0;
    virtual std::vector<std::string> GetAllImagesUuid(const CT::get_images_request& req) = 0;
    //return 2 uuids for group markers (marker uuid + chat uuid), and one uuid for private markers (marker uuid)
    virtual std::array<std::string, 2> AddMarker(const CT::marker_info& info) = 0;
    virtual std::vector<CT::marker_info> GetAllMarkers() = 0;
    virtual std::multimap<std::string, std::string> GetAllPushTokens() = 0;
    virtual void UpdateVerifStatus(const std::string& email, CT::verification_result res) = 0;
	virtual CT::update_info_response::result UpdateInfo(const CT::update_info_request& req) = 0;
    virtual std::string GetUuidByAppId(const std::string& app_id) = 0;
    virtual bool SaveChatMessage(const CT::chat_message& msg) = 0;
    virtual std::vector<CT::chat_message> GetChatMessages(const std::string& chat_id) = 0;
    virtual std::vector<CT::chat_info> GetUserChats(const std::string& user_id) = 0;
    virtual std::vector<std::string> GetChatParts(const std::string& chat_id) = 0;
    virtual bool AddUserToChat(const std::string& chat_id, const std::string& user_id) = 0;
    virtual CT::marker_info GetMarkerInfo(const std::string& marker_uuid) = 0;
    virtual bool AddUserImage(const std::string& user_uuid, const std::string& image_uuid) = 0;
    virtual bool AddMarkerImage(const std::string& marker_uuid, const std::string& image_uuid) = 0;
};
