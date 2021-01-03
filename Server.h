#pragma once

#include <memory>

class DBInterface;
class MailerInterface;
class MainEndpointImpl;
class UserStorageInterface;
class FileManager

class Server
{
    std::shared_ptr<DBInterface> m_db;
    std::shared_ptr<MailerInterface> m_mailer;
    std::shared_ptr<MainEndpointImpl> m_main_ep;
    std::shared_ptr<UserStorageInterface> m_user_storage;
    std::shared_ptr<FileManager> m_file_manager;
public:
    void Start();
};
