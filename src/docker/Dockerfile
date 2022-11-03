FROM ubuntu:18.04

MAINTAINER Krystian Zlomek <k.zlomek@adblockplus.org>

# Settings used later on in Dockerfile.
ARG CI_USER="ci_user"
ARG WORK_DIR="/opt/ci"
ARG HOME="/home/ci_user"
ARG DEPOT_TOOLS="$HOME/depot_tools"
ARG TIMEZONE="Europe/Berlin"

# Adjust env.
ENV GOOGLE_PLAY_AGREE_LICENSE="1" \
    LC_CTYPE="en_US.UTF-8" \
    CHROME_DEVEL_SANDBOX="1" \
    DEPOT_TOOLS="$DEPOT_TOOLS"

# Set timezone.
RUN ln -snf /usr/share/zoneinfo/$TIMEZONE /etc/localtime && echo $TIMEZONE > /etc/timezone && \
    echo ttf-mscorefonts-installer msttcorefonts/accepted-mscorefonts-eula select true | debconf-set-selections

# Install Chromium build dependencies.
RUN echo "deb http://archive.ubuntu.com/ubuntu trusty multiverse" >> /etc/apt/sources.list && \
    apt-get update && \
    apt-get install software-properties-common -qy && \
    apt-get update && \
    apt-get install -qy \
               git \
               build-essential \
               clang \
               curl \
               lsb-core \
               sudo \
               p7zip-full \
               wget \
               python \
               pkg-config \
               dumb-init \
               --no-install-recommends

# Install gitlab runner.
RUN curl -L https://packages.gitlab.com/install/repositories/runner/gitlab-runner/script.deb.sh | bash
COPY pin-gitlab-runner.pref /etc/apt/preferences.d/pin-gitlab-runner.pref
RUN apt-get install -qyy gitlab-runner

# Create and configure CI user.
RUN adduser --gecos "" --disabled-password $CI_USER --home $HOME && \
    usermod -aG sudo $CI_USER && \
    echo "$CI_USER ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers && \
    mkdir $WORK_DIR && \
    chown -R $CI_USER:$CI_USER $WORK_DIR

# Install Chromium's depot_tools.
RUN git clone https://chromium.googlesource.com/chromium/tools/depot_tools.git $DEPOT_TOOLS && \
    chown -R $CI_USER:$CI_USER $DEPOT_TOOLS

COPY lets-encrypt-x3-cross-signed.pem.txt /usr/local/share/ca-certificates/lets-encrypt-x3-cross-signed.ca.crt
RUN dpkg-reconfigure -f noninteractive ca-certificates

COPY run.sh /root/run.sh

ENTRYPOINT ["/usr/bin/dumb-init", "--"]

CMD ["/bin/bash", "/root/run.sh"]
