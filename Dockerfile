FROM alpine

RUN apk add gcc musl-dev git

RUN mkdir /work
WORKDIR /work
COPY build.sh main.c /work/

RUN sh build.sh

RUN mkdir /app

RUN cp ubik /app
