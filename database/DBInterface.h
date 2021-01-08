#pragma once
#include "../grpc/come_together.pb.h"

namespace CT = come_together_grpc;

class DBInterface
{
public:
    using Ptr = std::shared_ptr<DBInterface>;
    virtual ~DBInterface() = default;
    virtual CT::check_response::result Check(const CT::check_request& req) = 0;
    virtual CT::register_response::result RegisterUser(const CT::register_request& req, std::string& out_uuid) = 0;
    virtual CT::login_response::result LoginUser(const CT::login_request& req, std::string& out_uuid) = 0;
    virtual CT::user_info GetUserInfo(const std::string& user_uuid) = 0;
    virtual std::vector<std::string> GetAllImagesUuid(const CT::get_images_request& req) = 0;
    //return 2 uuids for group markers (marker uuid + chat uuid), and one uuid for private markers (marker uuid)
    virtual std::array<std::string, 2> AddMarker(const CT::marker_info& info) = 0;
    virtual std::vector<CT::marker_info> GetAllMarkers() = 0;
};
