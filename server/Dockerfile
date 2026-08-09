# Immagine dell'API server REST (server/): cpp-httplib + nlohmann/json + jwt-cpp + libpqxx
FROM ubuntu:24.04

LABEL maintainer="Marco Zanella <marco.zanella@unipd.it>"
LABEL description="API server REST del progetto (Programmazione a Oggetti, Università di Padova)"

# Evita prompt interattivi
ENV DEBIAN_FRONTEND=noninteractive

# Dipendenze dichiarate:
#   - catch2             (test del server, unit test senza DB)
#   - nlohmann-json3-dev (serializzazione JSON)
#   - libpqxx-dev        (PostgreSQL)
#   - libcrypt-dev       (bcrypt, propagato da db_repository)
#   - git                (FetchContent di cpp-httplib e jwt-cpp, header-only)
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    catch2 \
    nlohmann-json3-dev \
    libpqxx-dev \
    libcrypt-dev \
    && apt-get clean && rm -rf /var/lib/apt/lists/*

# Il contesto di build è la radice del repository: servono model/ e db/
WORKDIR /app
COPY model/ model/
COPY db/ db/
COPY server/ server/

# Compila l'API server e i suoi test, poi esegue i test (nessun DB richiesto)
RUN cmake -S server -B server/build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build server/build -j \
    && ctest --test-dir server/build --output-on-failure

EXPOSE 8080
CMD ["/app/server/build/api_server"]
