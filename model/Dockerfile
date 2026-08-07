# Immagine per compilare i test del modello logico e generare la documentazione
# (nessuna dipendenza GUI: il modello è una libreria C++ senza Qt)
FROM ubuntu:24.04

LABEL maintainer="Marco Zanella <marco.zanella@unipd.it>"
LABEL description="Compila i test del modello logico e genera la documentazione con Doxygen (corso di Programmazione a Oggetti, Università di Padova)"

# Evita prompt interattivi
ENV DEBIAN_FRONTEND=noninteractive

# Strumenti minimi: compilazione (cmake + gcc), framework di test (Catch2),
# formattazione (clang-format), generazione docs (doxygen + graphviz per i grafi)
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    catch2 \
    clang-format \
    doxygen \
    graphviz \
    && apt-get clean && rm -rf /var/lib/apt/lists/*

# Modello logico
WORKDIR /app
COPY . .

# Comando predefinito: compila i test, li esegue e genera la documentazione.
# - la build va in /tmp/build (non montato) per evitare la cache CMake dell'host
# - docs/ viene rigenerata da zero
# - la documentazione viene generata anche se i test falliscono, ma il comando
#   termina con il codice d'uscita dei test
CMD ["/bin/bash", "-c", "rm -rf /tmp/build docs && cmake -B /tmp/build && cmake --build /tmp/build -j && ctest --test-dir /tmp/build --output-on-failure; rc=$?; doxygen Doxyfile; exit $rc"]
