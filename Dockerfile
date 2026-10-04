FROM ubuntu:22.04

LABEL maintainer="Samuel Loza <samuel.loza26@gmail.com>"
LABEL version="1.0"

ENV DEBIAN_FRONTEND=noninteractive
ENV PYTHON_VERSION=3.12.0

WORKDIR /usr/src/app/onlinejudge-kernel

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    make \
    flex \
    g++ \
    default-libmysqlclient-dev \
    default-mysql-client \
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
    libssl-dev \
    zlib1g-dev \
    libncurses5-dev \
    libncursesw5-dev \
    libreadline-dev \
    libsqlite3-dev \
    libgdbm-dev \
    libdb5.3-dev \
    libbz2-dev \
    libexpat1-dev \
    liblzma-dev \
    libffi-dev \
    uuid-dev ca-certificates && \
    apt-get clean && rm -rf /var/lib/apt/lists/*


RUN locale-gen es_ES.UTF-8 && update-locale LANG=es_ES.UTF-8

# Instalar Python
RUN wget --no-check-certificate https://www.python.org/ftp/python/$PYTHON_VERSION/Python-$PYTHON_VERSION.tgz && \
    tar xvf Python-$PYTHON_VERSION.tgz && \
    cd Python-$PYTHON_VERSION && \
    ./configure && \
    make -j$(nproc) && \
    make altinstall && \
    cd .. && rm -rf Python-$PYTHON_VERSION*

# Verifica la instalación
RUN python3.12 --version

# Instalar Adoptium JDK 8
RUN mkdir -p /etc/apt/keyrings && \
    curl -fsSL --insecure https://packages.adoptium.net/artifactory/api/gpg/key/public | tee /etc/apt/keyrings/adoptium.asc && \
    echo "deb [signed-by=/etc/apt/keyrings/adoptium.asc] https://packages.adoptium.net/artifactory/deb $(awk -F= '/^VERSION_CODENAME/{print $2}' /etc/os-release) main" | tee /etc/apt/sources.list.d/adoptium.list && \
    apt-get update && \
    apt-get install -y temurin-8-jdk && \
    rm -rf /var/lib/apt/lists/*

RUN update-alternatives --set java /usr/lib/jvm/temurin-8-jdk-amd64/bin/java && \
    update-alternatives --set javac /usr/lib/jvm/temurin-8-jdk-amd64/bin/javac

# Instalar NVM, Node y Dolos. Symlink node/npm/dolos into /usr/local/bin so
# non-login processes launched by judged can run anti_cheating without Docker-in-Docker.
ARG NVM_VERSION=v0.39.7
ARG NODE_VERSION=22
RUN curl -o- https://raw.githubusercontent.com/nvm-sh/nvm/$NVM_VERSION/install.sh | bash && \
    export NVM_DIR="$HOME/.nvm" && \
    [ -s "$NVM_DIR/nvm.sh" ] && . "$NVM_DIR/nvm.sh" && \
    nvm install "$NODE_VERSION" && \
    nvm alias default "$NODE_VERSION" && \
    npm i -g @dodona/dolos && \
    NODE_BIN_DIR="$NVM_DIR/versions/node/$(nvm version)/bin" && \
    ln -sf "$NODE_BIN_DIR/node" /usr/local/bin/node && \
    ln -sf "$NODE_BIN_DIR/npm" /usr/local/bin/npm && \
    ln -sf "$NODE_BIN_DIR/npx" /usr/local/bin/npx && \
    ln -sf "$NODE_BIN_DIR/dolos" /usr/local/bin/dolos && \
    node --version && npm --version && command -v dolos

# Instalar cJSON
RUN git clone https://github.com/DaveGamble/cJSON.git /tmp/cJSON && \
    mkdir /tmp/cJSON/build && cd /tmp/cJSON/build && \
    cmake .. -DCMAKE_INSTALL_PREFIX=/usr && \
    make && make install && \
    rm -rf /tmp/cJSON

RUN useradd -m -u 1536 -s /bin/bash judge

COPY . /usr/src/app/onlinejudge-kernel

RUN chown -R judge:judge /usr/src/app/onlinejudge-kernel

RUN cd /usr/src/app/onlinejudge-kernel && ./install.sh

RUN chmod +x /usr/bin/pseint /usr/bin/psexport /usr/bin/anti_cheating.sh /etc/init.d/judged && \
    mkdir -p /home/judge /var/run && \
    chown -R judge:judge /home/judge && chmod -R 755 /home/judge

ENV LANG=en_US.UTF-8
ENV HOME=/home/judge
ENV CONF=/home/judge/etc/judge.conf

COPY ./docker/entrypoint.sh /usr/local/bin/entrypoint.sh

RUN chmod +x /usr/local/bin/entrypoint.sh

RUN rm -rf /usr/src/app/onlinejudge-kernel

RUN ln -sf /usr/local/bin/python3.12 /usr/bin/python3.12

USER root

WORKDIR /home/judge

ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]
