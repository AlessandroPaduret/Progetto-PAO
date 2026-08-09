#include "db/PasswordHasher.h"

#include <array>
#include <cstdio>
#include <random>
#include <stdexcept>
#include <string>

#include <crypt.h>

namespace db {

namespace {
constexpr int kBcryptRounds = 12;

// Alfabeto base64 di bcrypt (./A-Za-z0-9).
constexpr char kBcryptAlphabet[] =
    "./ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

// Genera un salt bcrypt "a mano" ($2b$<cost>$<22 char>), evitando
// crypt_gensalt() che su libxcrypt restituisce un buffer statico condiviso
// (non liberabile e non thread-safe).
std::string makeSalt() {
    std::array<unsigned char, 16> bytes;
    std::random_device rng;
    for (auto& b : bytes) {
        b = static_cast<unsigned char>(rng());
    }

    std::string salt = "$2b$";
    char cost[4];
    std::snprintf(cost, sizeof(cost), "%02d$", kBcryptRounds);
    salt += cost;

    // 16 byte -> 22 caratteri: 5 gruppi da 3 byte (20 char) + 1 byte (2 char).
    for (std::size_t i = 0; i < 15; i += 3) {
        unsigned b0 = bytes[i], b1 = bytes[i + 1], b2 = bytes[i + 2];
        salt += kBcryptAlphabet[b0 >> 2];
        salt += kBcryptAlphabet[((b0 & 0x03u) << 4) | (b1 >> 4)];
        salt += kBcryptAlphabet[((b1 & 0x0fu) << 2) | (b2 >> 6)];
        salt += kBcryptAlphabet[b2 & 0x3fu];
    }
    unsigned last = bytes[15];
    salt += kBcryptAlphabet[last >> 2];
    salt += kBcryptAlphabet[(last & 0x03u) << 4];
    return salt;
}

std::string cryptBcrypt(const std::string& password, const std::string& salt) {
    struct crypt_data data {};
    char* result = crypt_r(password.c_str(), salt.c_str(), &data);
    if (result == nullptr) {
        throw std::runtime_error("bcrypt: crypt_r failed");
    }
    return std::string(result);
}
} // namespace

std::string PasswordHasher::hash(const std::string& password) {
    return cryptBcrypt(password, makeSalt());
}

bool PasswordHasher::verify(const std::string& password,
                            const std::string& bcryptHash) {
    if (bcryptHash.rfind("$2", 0) != 0) {
        return false;
    }
    struct crypt_data data {};
    char* result = crypt_r(password.c_str(), bcryptHash.c_str(), &data);
    return result != nullptr && bcryptHash == std::string(result);
}

} // namespace db
