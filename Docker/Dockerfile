FROM besio/builder as builder
ARG branch=master
ARG symbol=SYS

RUN git clone -b $branch https://github.com/bitbesorg/BitBES.git --recursive \
    && cd bes && echo "$branch:$(git rev-parse HEAD)" > /etc/besio-version \
    && cmake -H. -B"/tmp/build" -GNinja -DCMAKE_BUILD_TYPE=Release -DWASM_ROOT=/opt/wasm -DCMAKE_CXX_COMPILER=clang++ \
       -DCMAKE_C_COMPILER=clang -DCMAKE_INSTALL_PREFIX=/tmp/build  -DSecp256k1_ROOT_DIR=/usr/local -DBUILD_MONGO_DB_PLUGIN=true -DCORE_SYMBOL_NAME=$symbol \
    && cmake --build /tmp/build --target install && rm /tmp/build/bin/besiocpp


FROM ubuntu:18.04

RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get -y install openssl && rm -rf /var/lib/apt/lists/*
COPY --from=builder /usr/local/lib/* /usr/local/lib/
COPY --from=builder /tmp/build/bin /opt/besio/bin
COPY --from=builder /tmp/build/contracts /contracts
COPY --from=builder /bes/Docker/config.ini /
COPY --from=builder /etc/besio-version /etc
COPY --from=builder /bes/Docker/nodbesd.sh /opt/besio/bin/nodbesd.sh
ENV BESIO_ROOT=/opt/besio
RUN chmod +x /opt/besio/bin/nodbesd.sh
ENV LD_LIBRARY_PATH /usr/local/lib
VOLUME /opt/besio/bin/data-dir
ENV PATH /opt/besio/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
