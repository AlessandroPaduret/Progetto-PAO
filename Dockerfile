# unipd-oop/qt-env:2025
FROM ubuntu:24.04

LABEL maintainer="Marco Zanella <marco.zanella@unipd.it>"
LABEL description="Ambiente standard per il corso di Programmazione a Oggetti, Università di Padova"

# Evita prompt interattivi
ENV DEBIAN_FRONTEND=noninteractive

# Aggiorna e installa tool essenziali
RUN apt-get update && apt-get install -y \
    build-essential \
    gcc \
    cmake \
    git \
    # Core Qt
    qt6-base-dev \
    qt6-tools-dev \
    qt6-tools-dev-tools \
    qt6-base-dev-tools \
    qt6-declarative-dev \
    # GUI extras
    qt6-svg-dev \
    qt6-charts-dev \
    # Multimedia
    qt6-multimedia-dev \
    # Database
    libsqlite3-dev \
    # (optional, useful for themes/localization)
    qt6-l10n-tools \
    && apt-get clean && rm -rf /var/lib/apt/lists/*

# Crea utente non root (opzionale ma utile)
RUN useradd -m student
USER student
WORKDIR /home/student

# Comando predefinito: apre shell
CMD ["/bin/bash"]

