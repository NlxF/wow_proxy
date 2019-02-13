FROM ubuntu:bionic

RUN apt update && apt install -y git cmake make gcc g++ clang libssl-dev python2.7

ADD wow_proxy /azerothcore/wow_proxy

RUN cd /azerothcore/wow_proxy; \
    /usr/bin/python2.7 emake.py -i; \
    emake make.mak;
