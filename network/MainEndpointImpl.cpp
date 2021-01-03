#include "MainEndpointImpl.h"
#include "QueueEvents.h"
#include "../core/UserStorageInterface.h"
#include "../database/DBInterface.h"
#include "../grpc/come_together.grpc.pb.h"
#include "../grpc/come_together.pb.h"
#include "../utils/FileManager.h"
#include "../utils/MailerInteface.h"
#include "CommonCallData.h"

#include <plog/Log.h>

MainEndpointImpl::MainEndpointImpl(DBInterface::Ptr db, std::shared_ptr<MailerInterface> mailer,
    std::unique_ptr<grpc_impl::ServerCompletionQueue> cq, std::shared_ptr<UserStorageInterface> storage,
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
}

MainEndpointImpl::~MainEndpointImpl()
{
    for(auto& thread : m_event_senders)
        thread.join();
}

::grpc::Status MainEndpointImpl::AskToken(::grpc::ServerContext* context, const CT::ask_token_request* request,
    CT::ask_token_response* response)
{
    CT::ask_token_response result;
    result.set_res(m_mailer->SendToken(request->email()));
    *response = std::move(result);
    PLOG_VERBOSE << "\npeer: " << context->peer() << "\nIN:\n" << request->Utf8DebugString() << "\nOUT:\n" << response->Utf8DebugString();
    return grpc::Status::OK;
}

::grpc::Status MainEndpointImpl::VerifyToken(::grpc::ServerContext* context, const CT::verify_token_request* request,
    CT::verify_token_response* response)
{
    CT::verify_token_response result;
    result.set_res(m_mailer->VerifyToken(request->email(), request->token()));
    *response = std::move(result);
    PLOG_VERBOSE << "\npeer: " << context->peer() << "\nIN:\n" << request->Utf8DebugString() << "\nOUT:\n" << response->Utf8DebugString();
    return grpc::Status::OK;
}

::grpc::Status MainEndpointImpl::Check(::grpc::ServerContext* context, const CT::check_request* request,
    CT::check_response* response)
{
    CT::check_response result;
    result.set_res(m_db->Check(*request));
    *response = std::move(result);
    PLOG_VERBOSE << "\npeer: " << context->peer() << "\nIN:\n" << request->Utf8DebugString() << "\nOUT:\n" << response->Utf8DebugString();
    return grpc::Status::OK;
}

::grpc::Status MainEndpointImpl::RegisterUser(::grpc::ServerContext* context, const CT::register_request* request,
    CT::register_response* response)
{
    CT::register_response result;
    std::string user_uuid;
    result.set_res(m_db->RegisterUser(*request, user_uuid));
    if (result.res() == CT::register_response_result_OK)
    {
        auto token = m_user_storage->LoginUser(user_uuid);
        result.set_access_token(std::move(token));
    }

    *response = std::move(result);
    PLOG_VERBOSE << "\npeer: " << context->peer() << "\nIN:\n" << request->Utf8DebugString() << "\nOUT:\n" << response->Utf8DebugString();
    return grpc::Status::OK;
}

::grpc::Status MainEndpointImpl::LoginUser(::grpc::ServerContext* context, const CT::login_request* request,
    CT::login_response* response)
{
    CT::login_response result;
    std::string user_uuid;
    result.set_res(m_db->LoginUser(*request, user_uuid));
    if (result.res() != CT::login_response_result_OK)
    {
        PLOG_INFO << context->peer() << ", login res=" << result.res();
        *response = std::move(result);
        return grpc::Status::OK;
    }
    assert(!user_uuid.empty());
    const auto user_info = new CT::user_info(m_db->GetUserInfo(user_uuid));
    result.set_allocated_info(user_info);
    auto access_token = m_user_storage->LoginUser(user_uuid);
    result.set_access_token(std::move(access_token));
    *response = std::move(result);
    auto common_call_data = new CommonCallData;
    common_call_data->ConnectToNewSubscriber([storage = m_user_storage](CommonCallData&& data)
    {
        storage->OnNewEventsSubscriber(std::move(data));
    });
    RequestSubscribeToEvents(common_call_data->m_ctx.get(), common_call_data->m_token.get(),
        common_call_data->m_writer.get(), m_cq.get(), m_cq.get(), common_call_data);
    PLOG_VERBOSE << "\npeer: " << context->peer() << "\nIN:\n" << request->Utf8DebugString() << "\nOUT:\n" << response->Utf8DebugString();
    return grpc::Status::OK;
}

::grpc::Status MainEndpointImpl::AddMarker(::grpc::ServerContext* context, const CT::add_marker_request* request,
    CT::add_marker_response* response)
{
    return grpc::Status::OK;
}

::grpc::Status MainEndpointImpl::GetAllMarkers(::grpc::ServerContext* context, const CT::access_token* request,
    ::grpc::ServerWriter<CT::marker_info>* writer)
{
    return grpc::Status::OK;
}

::grpc::Status MainEndpointImpl::GetInfo(::grpc::ServerContext* context, const CT::get_info_request* request,
    CT::get_info_response* response)
{
    return grpc::Status::OK;
}

::grpc::Status MainEndpointImpl::UpdateInfo(::grpc::ServerContext* context, const CT::update_info_request* request,
    CT::update_info_response* response)
{
    return grpc::Status::OK;
}

::grpc::Status MainEndpointImpl::ManageImage(::grpc::ServerContext* context, const CT::manage_image_request* request,
    CT::manage_image_response* response)
{
    return grpc::Status::OK;
}

::grpc::Status MainEndpointImpl::GetImages(::grpc::ServerContext* context, const CT::get_images_request* request,
    ::grpc::ServerWriter<CT::image>* writer)
{
    const auto images_uuid = m_db->GetAllImagesUuid(*request);
    PLOG_INFO_IF(images_uuid.empty()) << "no images for uuid=" << request->target_uuid() << ", req_type=" << request->type();
    CT::image img;
    for (const auto& file_name : images_uuid)
    {
        img.set_uuid(file_name);
        img.set_data(m_file_manager->GetFile(file_name));
        writer->Write(img);
    }
    PLOG_VERBOSE << "\npeer: " << context->peer() << "\nIN:\n" << request->Utf8DebugString() << "\nOUT:\nimages_uuid.size()=" << images_uuid.size();
    return grpc::Status::OK;
}

::grpc::Status MainEndpointImpl::SendChatMessage(::grpc::ServerContext* context,
    const CT::send_chat_message_request* request, CT::send_chat_message_response* response)
{
    return grpc::Status::OK;
}

::grpc::Status MainEndpointImpl::GetChatMessages(::grpc::ServerContext* context,
    const CT::get_chat_messages_request* request, ::grpc::ServerWriter<CT::chat_message>* writer)
{
    return grpc::Status::OK;
}

void MainEndpointImpl::SubscribeToNewEventsListener(grpc::ServerAsyncWriter<CT::event>* writer, grpc::ServerContext* ctx,
    ComeTogether::access_token* token, void* tag)
{
    RequestSubscribeToEvents(ctx, token, writer, m_cq.get(), m_cq.get(), tag);
}
