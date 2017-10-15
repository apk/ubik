FROM gcc

RUN mkdir /work
WORKDIR /work
COPY build.sh main.c /work/

RUN sh build.sh
