FROM besio/builder
ARG branch=master
ARG symbol=SYS

RUN git clone -b $branch https://github.com/bitbesorg/BitBES.git --recursive \
    && cd bes && echo "$branch:$(git rev-parse HEAD)" > /etc/besio-version \
    && cmake -H. -B"/opt/besio" -GNinja -DCMAKE_BUILD_TYPE=Release -DWASM_ROOT=/opt/wasm -DCMAKE_CXX_COMPILER=clang++ \
       -DCMAKE_C_COMPILER=clang -DCMAKE_INSTALL_PREFIX=/opt/besio  -DSecp256k1_ROOT_DIR=/usr/local -DBUILD_MONGO_DB_PLUGIN=true -DCORE_SYMBOL_NAME=$symbol \
    && cmake --build /opt/besio --target install \
    && mv /bes/Docker/config.ini / && mv /opt/besio/contracts /contracts && mv /bes/Docker/nodbesd.sh /opt/besio/bin/nodbesd.sh && mv tutorials /tutorials \
    && rm -rf /bes

RUN apt-get update && DEBIAN_FRONTEND=noninteractive apt-get -y install openssl ca-certificates vim psmisc python3-pip && rm -rf /var/lib/apt/lists/*
RUN pip3 install numpy
ENV BESIO_ROOT=/opt/besio
RUN chmod +x /opt/besio/bin/nodbesd.sh
ENV LD_LIBRARY_PATH /usr/local/lib
VOLUME /opt/besio/bin/data-dir
ENV PATH /opt/besio/bin:/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
