#ifndef DB_CONNECTION_POOL_H
#define DB_CONNECTION_POOL_H

#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <string>

#include <pqxx/pqxx>

namespace db {

/** @brief Pool thread-safe di connessioni libpqxx.
 *
 *  Le connessioni vengono create on-demand fino a `maxSize` e riutilizzate
 *  tramite RAII (`Lease`): ogni `acquire()` blocca fino a quando non è
 *  disponibile una connessione libera (o ne può essere creata una nuova).
 *  Da usare con `std::make_shared<ConnectionPool>(...)`.
 */
class ConnectionPool : public std::enable_shared_from_this<ConnectionPool> {
public:
    /** @brief RAII su una connessione del pool.
     *
     *  Restituisce automaticamente la connessione al pool alla distruzione;
     *  non è copiabile, solo spostabile.
     */
    class Lease {
    public:
        Lease(Lease&& other) noexcept;
        Lease& operator=(Lease&& other) noexcept;
        Lease(const Lease&) = delete;
        Lease& operator=(const Lease&) = delete;
        ~Lease();

        /** @brief Accesso alla connessione.
         *  @return Riferimento alla connessione libpqxx
         *  @throws std::logic_error se il lease è vuoto
         */
        pqxx::connection& get() const;

    private:
        friend class ConnectionPool;
        Lease(std::shared_ptr<ConnectionPool> pool,
              std::unique_ptr<pqxx::connection> conn);
        std::shared_ptr<ConnectionPool> m_pool;   /**< pool di provenienza */
        std::unique_ptr<pqxx::connection> m_conn; /**< connessione in uso */
    };

    /** @brief Costruttore.
     *  @param connString Stringa di connessione libpqxx (es. postgresql://user:pass@host:port/db)
     *  @param maxSize Numero massimo di connessioni aperte (default 8)
     */
    explicit ConnectionPool(std::string connString, std::size_t maxSize = 8);

    /** @brief Acquisisce una connessione dal pool.
     *  @return Un lease RAII sulla connessione
     *  @throws pqxx::connection_error se la creazione di una nuova connessione fallisce
     */
    Lease acquire();

private:
    friend class Lease;
    void release(std::unique_ptr<pqxx::connection> conn);

    std::string m_connString;      /**< stringa di connessione */
    std::size_t m_maxSize;         /**< limite di connessioni simultanee */
    std::mutex m_mutex;            /**< protezione dello stato del pool */
    std::condition_variable m_cv;  /**< notifica per la disponibilità di connessioni */
    std::queue<std::unique_ptr<pqxx::connection>> m_available; /**< connessioni libere */
    std::size_t m_total = 0;       /**< connessioni totali create */
};

} // namespace db

#endif // DB_CONNECTION_POOL_H
