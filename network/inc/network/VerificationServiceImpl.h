#pragma once

#include <string>

#include "come_together.grpc.pb.h"
#include "database/DBInterface.h"


class UserStorageInterface;
namespace CT = come_together_grpc;

class VerificationServiceImpl : public CT::VerificationService::Service
{
	std::unique_ptr<CT::VerificationService::Stub> m_stub;
	std::shared_ptr<UserStorageInterface> m_storage;
	DBInterface::Ptr m_db;
public:
	explicit VerificationServiceImpl(const std::string& ip_addr, std::shared_ptr<UserStorageInterface> storage,
		DBInterface::Ptr db);
	grpc::Status ValidatePhoto(grpc::ServerContext* context,
		grpc::ServerReader<come_together_grpc::validate_photo_request>* reader,
		come_together_grpc::validate_photo_response* response) override;
};