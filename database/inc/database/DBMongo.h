#pragma once
#include <boost/property_tree/ptree_fwd.hpp>
#include <mongocxx/database.hpp>
#include <mongocxx/client.hpp>
#include <mongocxx/instance.hpp>
#include "DBInterface.h"

class DBMongo : public DBInterface
{
    mongocxx::instance m_instance;
	mongocxx::client m_client;
	mongocxx::collection m_users_collection;
	mongocxx::collection m_chat_collection;
	mongocxx::collection m_markers_collection;
	public:
	explicit DBMongo(const boost::property_tree::ptree& config);
	CT::check_response::result Check(const CT::check_request& req) override;
	CT::register_response::result RegisterUser(const CT::register_request& req, std::string& out_uuid) override;
	CT::login_response::result LoginUser(const CT::login_request& req, std::string& out_uuid,
		std::string& out_access_token) override;
	CT::user_info GetUserInfo(const std::string& user_uuid) override;
	std::vector<std::string> GetAllImagesUuid(const CT::get_images_request& req) override;
	std::array<std::string, 2> AddMarker(const CT::marker_info& info) override;
	std::vector<CT::marker_info> GetAllMarkers() override;
	std::multimap<std::string, std::string> GetAllPushTokens() override;
};
