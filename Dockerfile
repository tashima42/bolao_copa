FROM debian:bookworm-slim

RUN apt-get update && apt-get install -y --no-install-recommends \
        gcc make git pkg-config ca-certificates \
        libwebsockets-dev \
        libmicrohttpd-dev \
        libssl-dev \
        zlib1g-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build
COPY . .
RUN make build

RUN mkdir -p /data
ENV ANSWERS_CSV=/data/answers.csv

ENTRYPOINT ["/build/bolao_copa"]
