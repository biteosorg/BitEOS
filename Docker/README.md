# Run in docker

Simple and fast setup of BitEOS on Docker is also available.

## Install Dependencies

- [Docker](https://docs.docker.com) Docker 17.05 or higher is required
- [docker-compose](https://docs.docker.com/compose/) version >= 1.10.0

## Docker Requirement

- At least 7GB RAM (Docker -> Preferences -> Advanced -> Memory -> 7GB or above)
- If the build below fails, make sure you've adjusted Docker Memory settings and try again.

## Build bes image

```bash
git clone https://github.com/bitbesorg/BitBES.git --recursive  --depth 1
cd bes/Docker
docker build . -t besio/bes
```

The above will build off the most recent commit to the master branch by default. If you would like to target a specific branch/tag, you may use a build argument. For example, if you wished to generate a docker image based off of the v1.0.1 tag, you could do the following:

```bash
docker build -t besio/bes:v1.0.1 --build-arg branch=v1.0.1 .
```

By default, the symbol in besio.system is set to SYS. You can override this using the symbol argument while building the docker image.

```bash
docker build -t besio/bes --build-arg symbol=<symbol> .
```

## Start nodbes docker container only

```bash
docker run --name nodbes -p 8888:8888 -p 9876:9876 -t besio/bes nodbesd.sh -e arg1 arg2
```

By default, all data is persisted in a docker volume. It can be deleted if the data is outdated or corrupted:

```bash
$ docker inspect --format '{{ range .Mounts }}{{ .Name }} {{ end }}' nodbes
fdc265730a4f697346fa8b078c176e315b959e79365fc9cbd11f090ea0cb5cbc
$ docker volume rm fdc265730a4f697346fa8b078c176e315b959e79365fc9cbd11f090ea0cb5cbc
```

Alternately, you can directly mount host directory into the container

```bash
docker run --name nodbes -v /path-to-data-dir:/opt/besio/bin/data-dir -p 8888:8888 -p 9876:9876 -t besio/bes nodbesd.sh -e arg1 arg2
```

## Get chain info

```bash
curl http://127.0.0.1:8888/v1/chain/get_info
```

## Start both nodbes and kbesd containers

```bash
docker volume create --name=nodbes-data-volume
docker volume create --name=kbesd-data-volume
docker-compose up -d
```

After `docker-compose up -d`, two services named `nodbesd` and `kbesd` will be started. nodbes service would expose ports 8888 and 9876 to the host. kbesd service does not expose any port to the host, it is only accessible to clbes when running clbes is running inside the kbesd container as described in "Execute clbes commands" section.

### Execute clbes commands

You can run the `clbes` commands via a bash alias.

```bash
alias clbes='docker-compose exec kbesd /opt/besio/bin/clbes -u http://nodbesd:8888 --wallet-url http://localhost:8888'
clbes get info
clbes get account inita
```

Upload sample exchange contract

```bash
clbes set contract exchange contracts/exchange/
```

If you don't need kbesd afterwards, you can stop the kbesd service using

```bash
docker-compose stop kbesd
```

### Develop/Build custom contracts

Due to the fact that the besio/bes image does not contain the required dependencies for contract development (this is by design, to keep the image size small), you will need to utilize the besio/bes-dev image. This image contains both the required binaries and dependencies to build contracts using besiocpp.

You can either use the image available on [Docker Hub](https://hub.docker.com/r/besio/bes-dev/) or navigate into the dev folder and build the image manually.

```bash
cd dev
docker build -t besio/bes-dev .
```

### Change default configuration

You can use docker compose override file to change the default configurations. For example, create an alternate config file `config2.ini` and a `docker-compose.override.yml` with the following content.

```yaml
version: "2"

services:
  nodbes:
    volumes:
      - nodbes-data-volume:/opt/besio/bin/data-dir
      - ./config2.ini:/opt/besio/bin/data-dir/config.ini
```

Then restart your docker containers as follows:

```bash
docker-compose down
docker-compose up
```

### Clear data-dir

The data volume created by docker-compose can be deleted as follows:

```bash
docker volume rm nodbes-data-volume
docker volume rm kbesd-data-volume
```

### Docker Hub

Docker Hub image available from [docker hub](https://hub.docker.com/r/besio/bes/).
Create a new `docker-compose.yaml` file with the content below

```bash
version: "3"

services:
  nodbesd:
    image: besio/bes:latest
    command: /opt/besio/bin/nodbesd.sh -e
    hostname: nodbesd
    ports:
      - 8888:8888
      - 9876:9876
    expose:
      - "8888"
    volumes:
      - nodbes-data-volume:/opt/besio/bin/data-dir

  kbesd:
    image: besio/bes:latest
    command: /opt/besio/bin/kbesd
    hostname: kbesd
    links:
      - nodbesd
    volumes:
      - kbesd-data-volume:/opt/besio/bin/data-dir

volumes:
  nodbes-data-volume:
  kbesd-data-volume:

```

*NOTE:* the default version is the latest, you can change it to what you want

run `docker pull besio/bes:latest`

run `docker-compose up`

### BESIO 1.0 Testnet

We can easily set up a BESIO 1.0 local testnet using docker images. Just run the following commands:

Note: if you want to use the mongo db plugin, you have to enable it in your `data-dir/config.ini` first.

```
# pull images
docker pull besio/bes:v1.0.1

# create volume
docker volume create --name=nodbes-data-volume
docker volume create --name=kbesd-data-volume
# start containers
docker-compose -f docker-compose-besio1.0.yaml up -d
# get chain info
curl http://127.0.0.1:8888/v1/chain/get_info
# get logs
docker-compose logs -f nodbesd
# stop containers
docker-compose -f docker-compose-besio1.0.yaml down
```

The `blocks` data are stored under `--data-dir` by default, and the wallet files are stored under `--wallet-dir` by default, of course you can change these as you want.

### About MongoDB Plugin

Currently, the mongodb plugin is disabled in `config.ini` by default, you have to change it manually in `config.ini` or you can mount a `config.ini` file to `/opt/besio/bin/data-dir/config.ini` in the docker-compose file.
