FROM alpine
RUN ak add --no-cache gcc

RUN mkdir /work
WORKDIR /work
COPY src h /work/

RUN sh build.sh
