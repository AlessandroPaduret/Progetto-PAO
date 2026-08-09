#include "auth.h"

#include <chrono>
#include <exception>
#include <string>

#include "jwt-cpp/jwt.h"

namespace server {

std::string createToken(long long userId, const std::string& secret) {
    return jwt::create()
        .set_issuer("pao")
        .set_subject(std::to_string(userId))
        .set_issued_at(std::chrono::system_clock::now())
        .set_expires_at(std::chrono::system_clock::now() + std::chrono::hours(24))
        .sign(jwt::algorithm::hs256{secret});
}

bool verifyToken(const std::string& token, const std::string& secret,
                 long long& userId) {
    try {
        auto decoded = jwt::decode(token);
        jwt::verify()
            .allow_algorithm(jwt::algorithm::hs256{secret})
            .verify(decoded);
        userId = std::stoll(std::string(decoded.get_subject()));
        return true;
    } catch (const std::exception&) {
        return false;
    }
}

} // namespace server
