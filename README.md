# Project Specifications
Project specification for the Object Oriented Programming class.


## Overview
This repository contains source LaTeX code for the specifications of the project for the Object Oriented Programming class.


## TO DO:

- repository(classe DB)
- GUI

## Docker

The `model/Dockerfile` provides a minimal environment (no GUI) that compiles the model tests (Catch2) and generates the Doxygen documentation. The `docker-compose.yml` at the repo root mounts `model/` and runs it:

```bash
# compila i test, li esegue e genera la documentazione in model/docs/
sudo systemctl start docker
docker compose up --build model
```

To open an interactive shell inside the image (e.g. to run `ctest` manually):

```bash
docker compose build model
docker run -it --rm -v "$(pwd)/model":/app -w /app -u $(id -u):$(id -g) pao-model:latest bash
```

nota: fai in modo di non essere connesso a eduroam o con tailscale altrimenti il DNS fa casino perché non trova Server DNS online
