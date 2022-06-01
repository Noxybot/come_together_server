#include "Server.h"

#include "core/UserStorageImpl.h"
#include "database/DBMongo.h"
#include "network/MainEndpointImpl.h"
#include "utils/FileManager.h"
#include "utils/Mailer.h"

#include <boost/property_tree/ini_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <grpc++/server_builder.h>


static const std::string SERVER_ADDRESS = "0.0.0.0:8080";

void Server::Start()
{
    boost::property_tree::ptree config;
   // read_ini("config.ini", config);
    m_db = std::make_shared<DBMongo>(config);
    m_mailer = std::make_shared<Mailer>(config);
    m_user_storage = std::make_shared<UserStorageImpl>();
    m_file_manager = std::make_shared<FileManager>("images/");
    grpc::ServerBuilder builder;
    builder.AddListeningPort(SERVER_ADDRESS, grpc::InsecureServerCredentials());
    builder.SetMaxMessageSize(std::numeric_limits<int>::max());
    builder.AddChannelArgument(GRPC_ARG_KEEPALIVE_PERMIT_WITHOUT_CALLS, true);
    auto cq = builder.AddCompletionQueue();
    m_main_ep = std::make_shared<MainEndpointImpl>(m_db, m_mailer, std::move(cq), m_user_storage, m_file_manager);
    builder.RegisterService(m_main_ep.get());
    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    m_main_ep->PostConstruct();
    server->Wait();
}
