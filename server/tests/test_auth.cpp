#include <string>

#include <catch2/catch_all.hpp>

#include "auth.h"

TEST_CASE("token JWT: round-trip corretto", "[auth]") {
    auto token = server::createToken(42, "secret");
    long long userId = 0;
    REQUIRE(server::verifyToken(token, "secret", userId));
    REQUIRE(userId == 42);
}

TEST_CASE("token JWT: segreto sbagliato", "[auth]") {
    auto token = server::createToken(42, "secret");
    long long userId = 0;
    REQUIRE_FALSE(server::verifyToken(token, "wrong", userId));
}

TEST_CASE("token JWT: token manomesso", "[auth]") {
    auto token = server::createToken(42, "secret");
    token[5] = 'X';
    long long userId = 0;
    REQUIRE_FALSE(server::verifyToken(token, "secret", userId));
}

TEST_CASE("token JWT: input vuoto o spazzatura", "[auth]") {
    long long userId = 0;
    REQUIRE_FALSE(server::verifyToken("", "secret", userId));
    REQUIRE_FALSE(server::verifyToken("non.un.token", "secret", userId));
}
