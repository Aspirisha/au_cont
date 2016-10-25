# eabatalov/aucont16-test-base container

FROM debian:stable
RUN apt-get -y update &&\
    apt-get install -y python3 &&\
    apt-get install -y sudo &&\
    apt-get install -y make &&\
    apt-get install -y gcc &&\
    apt-get install -y g++

RUN apt-get install -y libcap-dev

# Add user dev with uid 1000 and password 111
# Enter 111 when you run sudo in container
RUN groupadd dev && \
    useradd dev -g dev -u 1000 -G sudo -p sa72.Pi19MD8A

RUN apt-get install -y cmake && \
    apt-get install -y libstdc++6 && \
    apt-get install -y libboost-program-options-dev && \
    apt-get install -y python-twisted && \
    apt-get install -y cgroup-tools

VOLUME ["/test/aucont", "/test/scripts", "/test/rootfs"]
WORKDIR /test/scripts
USER 1000
CMD ./test.py
