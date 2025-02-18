FROM ubuntu:latest

LABEL maintainer="Samuel Loza <samuel.loza26@gmail.com>"
LABEL version="1.0"

ENV DEBIAN_FRONTEND=noninteractive

WORKDIR /usr/src/app/onlinejudge-kernel

RUN useradd -m -u 1536 -s /bin/bash judge

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
    locales nano\
    && apt clean && rm -rf /var/lib/apt/lists/*

RUN locale-gen es_ES.UTF-8 && update-locale LANG=es_ES.UTF-8

RUN mkdir -p /etc/apt/keyrings && \
    curl -fsSL https://packages.adoptium.net/artifactory/api/gpg/key/public | tee /etc/apt/keyrings/adoptium.asc && \
    echo "deb [signed-by=/etc/apt/keyrings/adoptium.asc] https://packages.adoptium.net/artifactory/deb $(awk -F= '/^VERSION_CODENAME/{print $2}' /etc/os-release) main" | \
    tee /etc/apt/sources.list.d/adoptium.list && \
    apt update && apt install -y temurin-8-jdk && apt clean

RUN update-alternatives --set java /usr/lib/jvm/temurin-8-jdk-amd64/bin/java && \
    update-alternatives --set javac /usr/lib/jvm/temurin-8-jdk-amd64/bin/javac

RUN curl -o- https://raw.githubusercontent.com/nvm-sh/nvm/v0.39.7/install.sh | bash && \
    bash -c 'source ~/.nvm/nvm.sh && nvm install node && npm i -g dolos'

RUN git clone https://github.com/DaveGamble/cJSON.git && \
    cd cJSON && mkdir build && cd build && cmake .. -DCMAKE_INSTALL_PREFIX=/usr && make && make install && cd ../../ && rm -rf cJSON

COPY ./docker/entrypoint.sh /usr/local/bin/entrypoint.sh

COPY . /usr/src/app/onlinejudge-kernel

RUN cd /usr/src/app/onlinejudge-kernel && ./install.sh

RUN chmod +x /usr/bin/pseint /usr/bin/psexport /usr/bin/anti_cheating.sh /etc/init.d/judged

RUN mkdir -p /home/judge/ && chown -R judge:judge /home/judge/ && chmod -R 755 /home/judge/

ENV LANG=es_ES.UTF-8
ENV HOME=/home/judge
ENV CONF=/home/judge/etc/judge.conf

RUN chmod +x /usr/local/bin/entrypoint.sh

USER judge

RUN rm -rf /usr/src/app/onlinejudge-kernel

WORKDIR /home/judge

ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]
