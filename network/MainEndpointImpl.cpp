#include "MainEndpointImpl.h"
#include "QueueEvents.h"
#include "core/UserStorageInterface.h"
#include "database/DBInterface.h"
#include "come_together.grpc.pb.h"
#include "come_together.pb.h"
#include "utils/FileManager.h"
#include "utils/MailerInteface.h"
#include "CommonCallData.h"

#include <plog/Log.h>

MainEndpointImpl::MainEndpointImpl(DBInterface::Ptr db, std::shared_ptr<MailerInterface> mailer,
                                   std::unique_ptr<grpc::ServerCompletionQueue> cq,
                                   std::shared_ptr<UserStorageInterface> storage,
                                   std::shared_ptr<FileManager> file_manager)
    : m_db(std::move(db))
      , m_mailer(std::move(mailer))
      , m_cq(std::move(cq))
      , m_user_storage(std::move(storage))
      , m_file_manager(std::move(file_manager))
{
    const auto cq_routine = [&]
    {
        while (true)
        {
            void* tag;
            bool ok;
            m_cq->Next(&tag, &ok);
            if (ok)
                static_cast<QueueEvents*>(tag)->OnAsyncEventFinished();
            else
                static_cast<QueueEvents*>(tag)->OnFinished();
        }
    };
    for (auto i = 0u; i < 1u; ++i)
        m_event_senders.emplace_back(std::thread(cq_routine));
    const auto markers_routing = [&]
    {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        auto markers = m_db->GetAllMarkers();
        std::cout << "sending " << markers.size() << " markers\n";
        for (auto& marker : markers) {
            auto event_ = std::make_shared<CT::event>();
            event_->set_event_type(CT::event_type_MARKER_ADDED);
            (*event_->mutable_m_info()) = std::move(marker);
            m_user_storage->PostToAllUsers(event_);
        }
    };
    /*std::thread{[&]
    {
        while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(10));
        PLOG_INFO << "sending message";
        auto evt = std::make_shared<CT::event>();
        evt->set_event_type(CT::event_type_MESSAGE_ADDED);
        CT::chat_message msg;
        msg.set_text("HELLO ALISA");
        msg.set_content_type(CT::chat_message_message_content_TEXT);
        *(evt->mutable_c_message()) = std::move(msg);
        m_user_storage->PostEventToUser("alisa.levadna@nure.ua", evt);
            }
    }
        
    }.detach();*/
}

void MainEndpointImpl::PostConstruct()
{
    WaitForEventsSubscriptionAsync();
}

MainEndpointImpl::~MainEndpointImpl()
{
    for (auto& thread : m_event_senders)
        thread.join();
}

::grpc::Status MainEndpointImpl::AskToken(::grpc::ServerContext* context, const CT::ask_token_request* request,
                                          CT::ask_token_response* response)
{
    CT::ask_token_response result;
    result.set_res(m_mailer->SendToken(request->email()));
    *response = std::move(result);
    PLOG_VERBOSE << "\npeer: " << context->peer() << "\nIN:\n" << request->Utf8DebugString() << "\nOUT:\n" << response->
Utf8DebugString();
    return grpc::Status::OK;
}

::grpc::Status MainEndpointImpl::VerifyToken(::grpc::ServerContext* context, const CT::verify_token_request* request,
                                             CT::verify_token_response* response)
{
    CT::verify_token_response result;
    result.set_res(m_mailer->VerifyToken(request->email(), request->token()));
    *response = std::move(result);
    PLOG_VERBOSE << "\npeer: " << context->peer() << "\nIN:\n" << request->Utf8DebugString() << "\nOUT:\n" << response->
Utf8DebugString();
    return grpc::Status::OK;
}

::grpc::Status MainEndpointImpl::Check(::grpc::ServerContext* context, const CT::check_request* request,
                                       CT::check_response* response)
{
    CT::check_response result;
    result.set_res(m_db->Check(*request));
    *response = std::move(result);
    PLOG_VERBOSE << "\npeer: " << context->peer() << "\nIN:\n" << request->Utf8DebugString() << "\nOUT:\n" << response->
Utf8DebugString();
    return grpc::Status::OK;
}

::grpc::Status MainEndpointImpl::RegisterUser(::grpc::ServerContext* context, const CT::register_request* request,
                                              CT::register_response* response)
{
    CT::register_response result;
    std::string user_uuid;
    result.set_res(m_db->RegisterUser(*request, user_uuid));
    result.set_user_uuid(std::move(user_uuid));
    *response = std::move(result);
    PLOG_VERBOSE << "\npeer: " << context->peer() << "\nIN:\n" << request->Utf8DebugString() << "\nOUT:\n" << response->
Utf8DebugString();
    return grpc::Status::OK;
}

::grpc::Status MainEndpointImpl::LoginUser(::grpc::ServerContext* context, const CT::login_request* request,
                                           CT::login_response* response)
{
    PLOG_VERBOSE << "\npeer: " << context->peer() << "\nIN:\n" << request->Utf8DebugString() << "\nOUT:\n" << response->
Utf8DebugString();
    CT::login_response result;
    std::string user_uuid, access_token;
    result.set_res(m_db->LoginUser(*request, user_uuid, access_token));
    if (result.res() != CT::login_response_result_OK)
    {
        PLOG_INFO << context->peer() << ", login res=" << result.res();
        *response = std::move(result);
        return grpc::Status::OK;
    }
    PLOG_ERROR_IF(user_uuid.empty()) << "empty user_uuid";
    *response = std::move(result);
    m_user_storage->LoginUser(user_uuid, request->app_id().id(), request->app_id().id());
    ////test code
    //auto t = [&]
    //{
    //    for (int i = 0; i < 10; ++i)
    //    {
    //        auto event_ = std::make_shared<CT::event>();
    //        event_->set_event_type(CT::event_type_MARKER_ADDED);
    //        auto info = new CT::marker_info;
    //        info->set_display_name(std::string("some_name#") + std::to_string(i));
    //        event_->set_allocated_m_info(info);
    //        m_user_storage->PostToAllUsers(event_);
    //        std::this_thread::sleep_for(std::chrono::seconds(2));
    //    }
    //};
    //std::thread{t}.detach();
    WaitForEventsSubscriptionAsync();
    return grpc::Status::OK;
}

::grpc::Status MainEndpointImpl::AddMarker(::grpc::ServerContext* context, const CT::add_marker_request* request,
                                           CT::add_marker_response* response)
{
    const auto res = m_db->AddMarker(request->info());
    auto event_ = std::make_shared<CT::event>();
    event_->set_event_type(CT::event_type_MARKER_ADDED);
    const auto info_copy = new CT::marker_info(request->info());
    info_copy->set_uuid(res[0]);
    info_copy->set_chat_uuid(res[1]);
    event_->set_allocated_m_info(info_copy);
    m_user_storage->PostToAllUsers(event_);
    response->set_uuid(res[0]);
    response->set_chat_uuid(res[1]);
    return grpc::Status::OK;
}

::grpc::Status MainEndpointImpl::GetAllMarkers(::grpc::ServerContext* context, const CT::access_token* request,
                                               ::grpc::ServerWriter<CT::marker_info>* writer)
{
    PLOG_INFO << "trying to get all markers";
    const auto markers = m_db->GetAllMarkers();
    for (const auto& marker : markers)
    {
        PLOG_INFO << "sending marker: " << marker.Utf8DebugString();
        writer->Write(marker);
    }
    return grpc::Status::OK;
}

::grpc::Status MainEndpointImpl::GetInfo(::grpc::ServerContext* context, const CT::get_info_request* request,
                                         CT::get_info_response* response)
{
    *response->mutable_u_info() = m_db->GetUserInfo(request->target_uuid());
    return grpc::Status::OK;
}

::grpc::Status MainEndpointImpl::UpdateInfo(::grpc::ServerContext* context, const CT::update_info_request* request,
                                            CT::update_info_response* response)
{
    CT::update_info_response res;
    res.set_res(m_db->UpdateInfo(*request));
    *response = std::move(res);
    return grpc::Status::OK;

}

::grpc::Status MainEndpointImpl::ManageImage(::grpc::ServerContext* context, const CT::manage_image_request* request,
                                             CT::manage_image_response* response)
{
    const auto& token = request->token().value();
    if (token.empty())
    {
        PLOG_ERROR << "empty access token";
        return grpc::Status::CANCELLED;
    }
    const auto user = m_user_storage->GetUuidByAccessToken(token);
    if (user.empty())
    {
        PLOG_ERROR << "user with token: " << token << " not subscribed to events";
        return grpc::Status::CANCELLED;
    }
    PLOG_INFO << "trying to save image";
    const auto& img = request->image();
    if (img.empty())
    {
        PLOG_ERROR << "image is empty";
        return grpc::Status::CANCELLED;
    }
    const auto file_uuid = m_file_manager->SaveFile(request->image());
    PLOG_INFO << "image uuid: " << file_uuid;
    if (request->act() == CT::manage_image_request_action_UPLOAD_USER_IMAGE)
        m_db->AddUserImage(user, file_uuid);
    else if (request->act() == CT::manage_image_request_action_UPLOAD_MARKER_IMAGE)
        m_db->AddMarkerImage(request->target_uuid(), file_uuid);
    response->set_res(CT::manage_image_response_result_OK);
    response->set_image_uuid(file_uuid);
    return grpc::Status::OK;
}

::grpc::Status MainEndpointImpl::GetImages(::grpc::ServerContext* context, const CT::get_images_request* request,
                                           ::grpc::ServerWriter<CT::image>* writer)
{
    const auto images_uuid = m_db->GetAllImagesUuid(*request);
    PLOG_INFO_IF(images_uuid.empty()) << "no images for uuid=" << request->target_uuid() << ", req_type=" << request->
image_type();
    CT::image img;
    for (const auto& file_name : images_uuid)
    {
        img.set_uuid(file_name);
        img.set_typ(CT::image_type_BINARY);
        img.set_binary(m_file_manager->GetFile(file_name));
        writer->Write(img);
    }
    PLOG_VERBOSE << "\npeer: " << context->peer() << "\nIN:\n" << request->Utf8DebugString() <<
 "\nOUT:\nimages_uuid.size()=" << images_uuid.size();
    return grpc::Status::OK;
}

::grpc::Status MainEndpointImpl::SendChatMessage(::grpc::ServerContext* context,
                                                 const CT::send_chat_message_request* request,
                                                 CT::send_chat_message_response* response)
{
    PLOG_INFO << request->Utf8DebugString();
    if (request->message().chat_uuid().empty())
    {
        PLOG_ERROR << "chat id is empty";
        return grpc::Status::CANCELLED;
    }
    m_db->SaveChatMessage(request->message());
    const auto parts = m_db->GetChatParts(request->message().chat_uuid());
    const auto evt = std::make_shared<CT::event>();
    evt->set_event_type(CT::event_type_MESSAGE_ADDED);
    *evt->mutable_c_message() = request->message();
    for (const auto& part : parts)
        m_user_storage->PostEventToUser(part, evt);
    return grpc::Status::OK;
}

::grpc::Status MainEndpointImpl::GetChatMessages(::grpc::ServerContext* context,
                                                 const CT::get_chat_messages_request* request,
                                                 ::grpc::ServerWriter<CT::chat_message>* writer)
{
    const auto msgs = m_db->GetChatMessages(request->chat_uuid());
    for (const auto& msg : msgs)
    {
        PLOG_INFO << "send msg from: " << msg.sender_uuid() << ", content: " << msg.text() << ", sent time: " << msg.sent_unix_time();
        writer->Write(msg);
    }
    return grpc::Status::OK;
}

::grpc::Status MainEndpointImpl::SendPushToken(::grpc::ServerContext* context,
                                               const CT::send_push_token_request* request,
                                               CT::send_push_token_response* response)
{
    PLOG_FATAL << request->push_token();
    return grpc::Status::OK;
}

grpc::Status MainEndpointImpl::JoinChat(grpc::ServerContext* context, const come_together_grpc::join_request* request,
	come_together_grpc::join_response* response)
{
    CT::join_response res;
    const auto user_uuid = m_user_storage->GetUuidByAccessToken(request->user_uuid());
    if (m_db->AddUserToChat(request->chat_uuid(), user_uuid))
        res.set_res(CT::join_response_result_OK);
    else
        res.set_res(CT::join_response_result_NOT_FOUND);
    *response = std::move(res);
    return grpc::Status::OK;
}

grpc::Status MainEndpointImpl::GetUserChats(grpc::ServerContext* context,
	const come_together_grpc::get_chats_request* request, grpc::ServerWriter<come_together_grpc::chat_info>* writer)
{
    PLOG_INFO << "in: " << request->Utf8DebugString();
    const auto& token = request->uuid();
    const auto user = m_user_storage->GetUuidByAccessToken(token);
    if (user.empty())
    {
        PLOG_ERROR << "cant find user for token: " << token;
        return grpc::Status::CANCELLED;
    }
    const auto chats = m_db->GetUserChats(user);
    for (const auto& chat : chats)
        writer->Write(chat);
    return grpc::Status::OK;
}

void MainEndpointImpl::WaitForEventsSubscriptionAsync()
{
    auto common_call_data = new CommonCallData;
    common_call_data->ConnectToNewSubscriber(
        [this, storage = m_user_storage, w_this = weak_from_this()](CommonCallData&& data)
        {
            storage->OnNewEventsSubscriber(std::move(data));
            if (const auto locked_this = w_this.lock())
                WaitForEventsSubscriptionAsync();
        });
    RequestSubscribeToEvents(common_call_data->m_ctx.get(), common_call_data->m_access_token.get(),
                             common_call_data->m_writer.get(), m_cq.get(), m_cq.get(), common_call_data);
}
