#include "db/ConnectionPool.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace db {

ConnectionPool::ConnectionPool(std::string connString, std::size_t maxSize)
    : m_connString(std::move(connString)),
      m_maxSize(std::max<std::size_t>(1, maxSize)) {}

ConnectionPool::Lease ConnectionPool::acquire() {
    std::unique_lock<std::mutex> lock(m_mutex);
    m_cv.wait(lock, [this] {
        return !m_available.empty() || m_total < m_maxSize;
    });

    if (!m_available.empty()) {
        auto conn = std::move(m_available.front());
        m_available.pop();
        return Lease(shared_from_this(), std::move(conn));
    }

    auto conn = std::make_unique<pqxx::connection>(m_connString);
    ++m_total;
    return Lease(shared_from_this(), std::move(conn));
}

void ConnectionPool::release(std::unique_ptr<pqxx::connection> conn) {
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_available.push(std::move(conn));
    }
    m_cv.notify_one();
}

ConnectionPool::Lease::Lease(std::shared_ptr<ConnectionPool> pool,
                             std::unique_ptr<pqxx::connection> conn)
    : m_pool(std::move(pool)), m_conn(std::move(conn)) {}

ConnectionPool::Lease::~Lease() {
    if (m_pool && m_conn) {
        m_pool->release(std::move(m_conn));
    }
}

pqxx::connection& ConnectionPool::Lease::get() const {
    if (!m_conn) {
        throw std::logic_error("Lease::get on null connection");
    }
    return *m_conn;
}

ConnectionPool::Lease::Lease(Lease&& other) noexcept
    : m_pool(std::move(other.m_pool)), m_conn(std::move(other.m_conn)) {}

ConnectionPool::Lease& ConnectionPool::Lease::operator=(Lease&& other) noexcept {
    if (this != &other) {
        m_pool = std::move(other.m_pool);
        m_conn = std::move(other.m_conn);
    }
    return *this;
}

} // namespace db
