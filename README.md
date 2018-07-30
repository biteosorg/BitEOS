# Bit EOS

<!-- vim-markdown-toc GFM -->

* [Supported Operating Systems](#supported-operating-systems)
* [Getting Started](#getting-started)
    * [Build from source](#build-from-source)
        * [Notes](#notes)
        * [Command reference](#command-reference)
    * [Docker](#docker)
    * [Seed list](#seed-list)
        * [P2P node](#p2p-node)
        * [Wallet node](#wallet-node)
* [Resources](#resources)

<!-- vim-markdown-toc -->

## Supported Operating Systems

Bit EOS currently supports the following operating systems:

1. Ubuntu 16.04 (Ubuntu 16.10 recommended)
2. macOS 10.12 and higher (macOS 10.13.x recommended)

## Getting Started

### Build from source

```bash
# clone this repository and build
$ git clone https://github.com/biteosorg/BitEOS BitEOS
$ cd BitEOS
$ ./besio_build.sh

# once you've built successfully
$ cd build && make install

# also copy the generated abi and wasm contract file to ~/.local/share/besio/nodbes/config
$ cp build/contracts/besio.token/besio.token.abi build/contracts/besio.token/besio.token.wasm ~/.local/share/besio/nodbes/config
$ cp build/contracts/System/System.abi build/contracts/System/System.wasm ~/.local/share/besio/nodbes/config

# place some p2p nodes in here, see the following seed list section
$ vim ~/.local/share/besio/nodbes/config/config.ini

# download genesis.json to ~/.local/share/besio/nodbes/config
$ curl https://raw.githubusercontent.com/biteosorg/genesis/master/genesis.json -o ~/.local/share/besio/nodbes/config/genesis.json

$ cd build/programs/nodbes && ./nodbes
```

#### Notes

:warning:

- Make sure your config directory has these 6 files:

    ```bash
    config
    ├── System.abi
    ├── System.wasm
    ├── besio.token.abi
    ├── besio.token.wasm
    ├── config.ini
    └── genesis.json
    ```

    - `config.ini` with `p2p-peer-address` added. You can find some seed nodes in the following P2P node section.

    - `genesis.json` from [github.com/biteosorg/genesis](https://raw.githubusercontent.com/biteosorg/genesis/master/genesis.json).

    - compiled contract files: `System.abi`, `System.wasm`, `besio.token.abi` and `besio.token.wasm` from `build/contracts/System` and `build/contracts/besio.token`.

- Ensure your chain id is correct when your node is up: `https://github.com/biteosorg/BitEOS`

#### Command reference

- [RPC interface](https://github.com/biteosorg/BitEOS)

- [CLI command reference](https://github.com/biteosorg/BitEOS)

### Docker

[Run a node via docker](https://github.com/biteosorg/BitEOS)

### Seed list

#### P2P node

These IPs could be used as `p2p-peer-address` in your `config.ini`:

IP                  | Domain Name                 | By
:----:              | :----:                      | :----:
N/A                 | N/A                         | N/A

#### Wallet node

The following IPs provide HTTP service for wallet.

IP                  | Domain Name            | By
:----:              | :----:                 | :----:
N/A                 | N/A                         | N/A

## Resources

1. [BitEOS Website](http://biteos.org)
2. [Community Telegram Group](http://biteos.org)
3. [Developer Telegram Group](http://biteos.org)
