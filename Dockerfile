FROM ubuntu:22.04

LABEL org.opencontainers.image.title="webserv"
LABEL org.opencontainers.image.description="A lightweight HTTP server implementation."
LABEL org.opencontainers.image.authors="Shotaro Mizuochi <smizuoch@student.42tokyo.jp>"
LABEL org.opencontainers.image.source="https://github.com/tobeshota/webserv"
LABEL org.opencontainers.image.documentation="https://github.com/tobeshota/webserv#readme"
LABEL org.opencontainers.image.version="1.0.3"

RUN apt-get update && apt-get install -y \
    gcc \
    g++ \
    clang-format \
    cmake \
    python3 \
    python3-pip \
    telnet \
    doxygen \
    graphviz \
    git \
    make \
    curl \
    lcov \
    siege \
    clang \
    llvm \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

RUN pip3 install pytest

WORKDIR /app

CMD ["bash"]
