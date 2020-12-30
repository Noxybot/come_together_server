#pragma once
#include "grpc/come_together.grpc.pb.h"
#include "grpc/come_together.pb.h"



namespace CT = ComeTogether;
using AsyncUpdatesEndpoint =  CT::MainEndpoint::WithAsyncMethod_SubscribeToEvents<CT::MainEndpoint::Service>;

class MainEndpoint : public AsyncUpdatesEndpoint
{
public:
    ::grpc::Status Check(::grpc::ServerContext* context, const CT::check_request* request,
        CT::check_response* response) override;
    ::grpc::Status RegisterUser(::grpc::ServerContext* context, const CT::register_request* request,
        CT::register_response* response) override;
    ::grpc::Status LoginUser(::grpc::ServerContext* context, const CT::login_request* request,
        CT::login_response* response) override;
    ::grpc::Status AddMarker(::grpc::ServerContext* context, const CT::add_marker_request* request,
        CT::add_marker_response* response) override;
    ::grpc::Status GetAllMarkers(::grpc::ServerContext* context, const CT::access_token* request,
        ::grpc::ServerWriter<CT::marker_info>* writer) override;
    ::grpc::Status GetInfo(::grpc::ServerContext* context, const CT::get_info_request* request,
        CT::get_info_response* response) override;
    ::grpc::Status UpdateInfo(::grpc::ServerContext* context, const CT::update_info_request* request,
        CT::update_info_response* response) override;
    ::grpc::Status ManageImage(::grpc::ServerContext* context, const CT::manage_image_request* request,
        CT::manage_image_response* response) override;
    ::grpc::Status GetImages(::grpc::ServerContext* context, const CT::get_images_request* request,
        ::grpc::ServerWriter<CT::image>* writer) override;
    ::grpc::Status SendChatMessage(::grpc::ServerContext* context,
        const CT::send_chat_message_request* request,
        CT::send_chat_message_response* response) override;
    ::grpc::Status GetChatMessages(::grpc::ServerContext* context,
        const CT::get_chat_messages_request* request,
        ::grpc::ServerWriter<CT::chat_message>* writer) override;
    ~MainEndpoint() override;
    ::grpc::Status SubscribeToEvents(::grpc::ServerContext*, const CT::access_token*,
        ::grpc::ServerWriter<CT::event>*) override;
};