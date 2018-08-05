FROM ubuntu:bionic

# RUN apt update && apt install -y git cmake make gcc g++ clang libssl-dev
# RUN useradd -m docker && echo "docker:docker" | chpasswd && adduser docker sudo
# USER docker

RUN apt update && apt -y install wget
RUN wget http://skywind3000.github.io/emake/emake.py
RUN python emake.py -i

ADD wow_proxy /azerothcore/wow_proxy

RUN cd /azerothcore/wow_proxy; \
    emake make.mak;
