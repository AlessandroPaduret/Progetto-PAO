#include <iostream>
#include <string>

#include "db/PasswordHasher.h"

#define CHECK(cond)                                                    \
    do {                                                               \
        if (!(cond)) {                                                 \
            std::cerr << "CHECK fallita: " #cond " (riga " << __LINE__ \
                      << ")\n";                                        \
            return 1;                                                  \
        }                                                              \
    } while (0)

int main() {
    const std::string password = "password123";

    std::string hash1 = db::PasswordHasher::hash(password);
    std::string hash2 = db::PasswordHasher::hash(password);

    // bcrypt: hash salato -> due hash della stessa password devono differire.
    CHECK(hash1 != hash2);
    CHECK(hash1.rfind("$2b$", 0) == 0);

    // Verifica positiva/negativa.
    CHECK(db::PasswordHasher::verify(password, hash1));
    CHECK(db::PasswordHasher::verify(password, hash2));
    CHECK(!db::PasswordHasher::verify("password124", hash1));

    std::cout << "bcrypt smoke OK: " << hash1 << "\n";
    return 0;
}
