#include "PushSender.h"

PushSender::PushSender(DBInterface::Ptr db)
    : m_db(std::move(db))
    , m_push_tokens(m_db->GetAllPushTokens())
{}
