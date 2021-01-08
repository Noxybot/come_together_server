#pragma once

#include "DBInterface.h"

#include <memory>

class ConnectionPool;

class DBPostgres : public DBInterface
{
    const std::shared_ptr<ConnectionPool> m_pool;
public:
    ~DBPostgres() override = default;
    explicit DBPostgres(const std::string& connection_string);
    CT::check_response::result Check(const CT::check_request& req) override;
    CT::register_response::result RegisterUser(const CT::register_request& req, std::string& out_uuid) override;
    CT::login_response::result LoginUser(const CT::login_request& req, std::string& out_uuid) override;
    CT::user_info GetUserInfo(const std::string& user_uuid) override;
    std::vector<std::string> GetAllImagesUuid(const CT::get_images_request& req) override;
    std::array<std::string, 2> AddMarker(const CT::marker_info& info) override;
    std::vector<CT::marker_info> GetAllMarkers() override;
};