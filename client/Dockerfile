FROM ubuntu:24.04

WORKDIR /opt/kcp_client

COPY include/ .
COPY lib/* .
COPY build/main .
ENV LD_LIBRARY_PATH="/opt/kcp_client/"

CMD ["./main"]

# build the docker image:
# docker build -t kcp_client .
# run the docker image:
# docker run --network host -it kcp_client