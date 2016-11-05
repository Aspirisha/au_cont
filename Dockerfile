# eabatalov/aucont16-test-base container

FROM eabatalov/aucont16-test-base

USER root

RUN apt-get install -y cmake && \
    apt-get install -y python-twisted && \
    apt-get install -y cgroup-tools

USER dev