#pragma once
#include <pqxx/pqxx>

#include <memory>
#include <set>
#include <mutex>

class ConnectionPool : public std::enable_shared_from_this<ConnectionPool>
{
    using conn_ptr = std::shared_ptr<pqxx::connection>;
    class ConnectionWrapper
    {
        std::shared_ptr<ConnectionPool> m_parent;
        conn_ptr m_conn;
    public:
        ConnectionWrapper() = default;
        ConnectionWrapper(std::shared_ptr<ConnectionPool> parent, conn_ptr conn);
        ~ConnectionWrapper() noexcept;
        ConnectionWrapper(const ConnectionWrapper&) = delete;
        const ConnectionWrapper& operator=(const ConnectionWrapper&) = delete;
        pqxx::connection& operator*() const;
    };
    std::mutex m_mtx;
    std::set<conn_ptr> m_free_connections;
    std::condition_variable m_condition;

public:
    ConnectionPool(const std::string& connection_string, std::size_t num_connections);
    ConnectionWrapper AcquireConnection(std::chrono::steady_clock::duration timeout = std::chrono::seconds(5));
    void ReleaseConnection(conn_ptr connection);
};
