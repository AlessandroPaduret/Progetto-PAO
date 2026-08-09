# Ambiente riproducibile per la libreria db_repository (persistenza + bcrypt).
# Compila la libreria, esegue lo smoke test bcrypt (nessun DB richiesto) e
# compila lo smoke test dei repository (richiede una PostgreSQL a runtime).
FROM ubuntu:24.04

LABEL maintainer="Marco Zanella <marco.zanella@unipd.it>"
LABEL description="Compila la libreria di persistenza del progetto (libpqxx + bcrypt) e i relativi smoke test"

ENV DEBIAN_FRONTEND=noninteractive

# Dipendenze dichiarate:
#   - libpqxx-dev  (C++/PostgreSQL)
#   - libcrypt-dev (bcrypt, schema $2b$)
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    libpqxx-dev \
    libcrypt-dev \
    && apt-get clean && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

CMD ["/bin/bash", "-c", "cmake -B build && cmake --build build -j && ctest --test-dir build --output-on-failure"]
