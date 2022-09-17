#include "VerificationServiceImpl.h"

#include <grpcpp/create_channel.h>
#include <grpcpp/security/credentials.h>
#include <plog/Log.h>

#include "core/UserStorageInterface.h"

VerificationServiceImpl::VerificationServiceImpl(const std::string& ip_addr, std::shared_ptr<UserStorageInterface> storage, DBInterface::Ptr db)
	: m_stub(CT::VerificationService::NewStub(grpc::CreateChannel(ip_addr, grpc::InsecureChannelCredentials())))
	, m_storage(std::move(storage))
	, m_db(std::move(db))
{
	
}

grpc::Status VerificationServiceImpl::ValidatePhoto(grpc::ServerContext* context,
                                                    grpc::ServerReader<come_together_grpc::validate_photo_request>* reader,
                                                    come_together_grpc::validate_photo_response* response)
{
	PLOG_VERBOSE << "peer: " << context->peer();
	CT::validate_photo_request req;
	grpc::ClientContext ctx;
	CT::validate_photo_response res;
	const auto writer = m_stub->ValidatePhoto(&ctx, &res);
	while (reader->Read(&req)){
		const auto& app_id = req.app_id().id();
		const auto uuid = m_storage->GetUuidByAccessToken(app_id);
		if (uuid.empty())
		{
			PLOG_ERROR << "app id " << app_id << "is not logged in";
			//return grpc::Status::CANCELLED;
		}
		PLOG_INFO << "app_id: " << app_id << " email: " << uuid;
		PLOG_INFO << "sending chunk  user_photo_size: " << req.user_photo().size() << ", valid size: " << req.validation_photo().size()
		<< ", user_complete: " << req.user_photo_complete() << ", valid complete: " << req.validation_photo_complete();
		writer->Write(req);
	}
	writer->Finish();
	(*response) = std::move(res);
	PLOG_VERBOSE << "out: " << response->Utf8DebugString();
	m_db->UpdateVerifStatus(req.app_id().id(), response->res());
	return grpc::Status::OK;
}
