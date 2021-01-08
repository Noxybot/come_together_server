#pragma once

#include "../database/DBInterface.h"

#include "../grpc/come_together.grpc.pb.h"
#include "../grpc/come_together.pb.h"

#include <memory>
#include <thread>
#include <vector>


namespace CT = come_together_grpc;
using AsyncUpdatesEndpoint =  CT::MainEndpoint::WithAsyncMethod_SubscribeToEvents<CT::MainEndpoint::Service>;
class MailerInterface;
class UserStorageInterface;
class FileManager;

class MainEndpointImpl : public AsyncUpdatesEndpoint, std::enable_shared_from_this<MainEndpointImpl>
{
    const DBInterface::Ptr m_db;
    const std::shared_ptr<MailerInterface> m_mailer;
    const std::unique_ptr<grpc_impl::ServerCompletionQueue> m_cq;
    const std::shared_ptr<UserStorageInterface> m_user_storage;
    const std::shared_ptr<FileManager> m_file_manager;
    std::vector<std::thread> m_event_senders;
public:
    explicit MainEndpointImpl(DBInterface::Ptr db, std::shared_ptr<MailerInterface>,
        std::unique_ptr<grpc_impl::ServerCompletionQueue> cq, std::shared_ptr<UserStorageInterface> storage,
        std::shared_ptr<FileManager> file_manager);
    ~MainEndpointImpl() override;
    ::grpc::Status AskToken(::grpc::ServerContext* context, const CT::ask_token_request* request,
        CT::ask_token_response* response) override;
    ::grpc::Status VerifyToken(::grpc::ServerContext* context, const CT::verify_token_request* request,
        CT::verify_token_response* response) override;
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
    ::grpc::Status GetChatMessages(::grpc::ServerContext* context, const CT::get_chat_messages_request* request,
        ::grpc::ServerWriter<CT::chat_message>* writer) override;
    ::grpc::Status SendPushToken(::grpc::ServerContext* context, const CT::send_push_token_request* request,
        CT::send_push_token_response* response) override;
private:
    void WaitForEventsSubscriptionAsync();
};