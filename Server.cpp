#include "Server.h"

#include "core/UserStorageImpl.h"
#include "database/DBPostgres.h"
#include "network/MainEndpointImpl.h"
#include "utils/FileManager.h"
#include "utils/Mailer.h"

#include <grpc++/server_builder.h>

static const std::string SERVER_ADDRESS = "0.0.0.0:53681";

void Server::Start()
{
    m_db = std::make_shared<DBPostgres>
    ("postgres://fzxmhwnqaxlqzo:fa9f5b5d940b09fcafc81de9daf27dfbb681c57c7e4a92222e00f8b2ef0f1d05"
     "@ec2-52-208-175-161.eu-west-1.compute.amazonaws.com:5432/dcfcatkkl391gn?sslmode=require&application_name=server");
    //m_db->GetUserInfo("4b5f5a40-15b6-4e0c-91ad-6b0f729d00fc");
    m_mailer = std::make_shared<Mailer>("noreply@come_together_grpc.com", "eduard.voronkin@nure.ua", "Andyou33");
    m_user_storage = std::make_shared<UserStorageImpl>();
    m_file_manager = std::make_shared<FileManager>("images/");
    grpc::ServerBuilder builder;
    builder.AddListeningPort(SERVER_ADDRESS, grpc::InsecureServerCredentials());
    builder.SetMaxMessageSize(std::numeric_limits<int>::max());
    builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS, true);
    auto cq = builder.AddCompletionQueue();
    m_main_ep = std::make_shared<MainEndpointImpl>(m_db, m_mailer, std::move(cq), m_user_storage, m_file_manager);
    builder.RegisterService(m_main_ep.get());
    std::unique_ptr<grpc_impl::Server> server(builder.BuildAndStart());
    server->Wait();
}
