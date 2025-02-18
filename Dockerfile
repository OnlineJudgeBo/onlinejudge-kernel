FROM ubuntu:22.04

LABEL maintainer="Samuel Loza <samuel.loza26@gmail.com>"
LABEL version="1.0"

ENV DEBIAN_FRONTEND=noninteractive

WORKDIR /usr/src/app/onlinejudge-kernel

RUN apt update && apt install -y \
    build-essential \
    make \
    flex \
    g++ \
    default-libmysqlclient-dev \
    libmysql++-dev \
    dos2unix \
    libcurl4-openssl-dev \
    curl \
    cmake \
    git \
    supervisor \
    wget \
    gnupg2 \
    locales \
    nano \
    && rm -rf /var/lib/apt/lists/*

RUN locale-gen es_ES.UTF-8 && update-locale LANG=es_ES.UTF-8

# Instalar Adoptium JDK 8
RUN mkdir -p /etc/apt/keyrings && \
    curl -fsSL https://packages.adoptium.net/artifactory/api/gpg/key/public | tee /etc/apt/keyrings/adoptium.asc && \
    echo "deb [signed-by=/etc/apt/keyrings/adoptium.asc] https://packages.adoptium.net/artifactory/deb $(awk -F= '/^VERSION_CODENAME/{print $2}' /etc/os-release) main" | \
    tee /etc/apt/sources.list.d/adoptium.list && \
    apt update && apt install -y temurin-8-jdk && rm -rf /var/lib/apt/lists/*

RUN update-alternatives --set java /usr/lib/jvm/temurin-8-jdk-amd64/bin/java && \
    update-alternatives --set javac /usr/lib/jvm/temurin-8-jdk-amd64/bin/javac

# Instalar NVM y Dolos
ARG NVM_VERSION=v0.39.7
RUN curl -o- https://raw.githubusercontent.com/nvm-sh/nvm/$NVM_VERSION/install.sh | bash && \
    export NVM_DIR="$HOME/.nvm" && \
    [ -s "$NVM_DIR/nvm.sh" ] && . "$NVM_DIR/nvm.sh" && \
    nvm install node && npm i -g dolos

# Instalar cJSON
RUN git clone https://github.com/DaveGamble/cJSON.git /tmp/cJSON && \
    mkdir /tmp/cJSON/build && cd /tmp/cJSON/build && \
    cmake .. -DCMAKE_INSTALL_PREFIX=/usr && \
    make && make install && \
    rm -rf /tmp/cJSON

COPY --chown=judge:judge . /usr/src/app/onlinejudge-kernel

RUN cd /usr/src/app/onlinejudge-kernel && ./install.sh

RUN chmod +x /usr/bin/pseint /usr/bin/psexport /usr/bin/anti_cheating.sh /etc/init.d/judged && \
    mkdir -p /home/judge/ && chown -R judge:judge /home/judge/ && chmod -R 755 /home/judge/

ENV LANG=en_US.UTF-8
ENV HOME=/home/judge
ENV CONF=/home/judge/etc/judge.conf

COPY ./docker/entrypoint.sh /usr/local/bin/entrypoint.sh
RUN chmod +x /usr/local/bin/entrypoint.sh

RUN useradd -m -u 1536 -s /bin/bash judge

USER judge

RUN rm -rf /usr/src/app/onlinejudge-kernel

WORKDIR /home/judge

ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]
