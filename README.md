# Project Specifications
Project specification for the Object Oriented Programming class.


## Overview
This repository contains source LaTeX code for the specifications of the project for the Object Oriented Programming class.


## TO DO:

- repository(classe DB)
- GUI

## Docker Image
To build the Docker image run
```bash
docker build -t unipd-oop/qt-env:2025 .
```
nota: fai in modo di non essere connesso a eduroam o con tailscale altrimenti il DNS fa casino perché non trova Server DNS online

# Attiva il servizio docker
sudo systemctl start docker

To run an interactive shell run
```bash
docker run -it --rm \
  -v "$(pwd)":/app -w /app \
  -u $(id -u):$(id -g) \
  unipd-oop/qt-env:2025 bash
```
this mount the current directory as `/app` and sets appropriate user permissions.

To run an interactive shell with access to graphic support (necessary to run the application) on GNU/Linux run
```bash
# Only once
xhost +local:docker

docker run -it --rm \
  -v "$(pwd)":/app -w /app \
  -u $(id -u):$(id -g) \
  -e DISPLAY=$DISPLAY \
  -e XDG_RUNTIME_DIR=$XDG_RUNTIME_DIR \
  -v $XDG_RUNTIME_DIR:$XDG_RUNTIME_DIR \
  -v /tmp/.X11-unix:/tmp/.X11-unix \
  unipd-oop/qt-env:2025 bash
  ```
