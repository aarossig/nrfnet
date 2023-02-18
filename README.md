# nerfnet

```
TODO(aarossig): Update this readme with new mesh documentation.
```

## build

This project uses the cmake build system and has some dependencies.

```
sudo apt-get install git cmake build-essential \
    libtclap-dev libgtest-dev \
    libprotobuf-dev protobuf-compiler
```

The RF24 library must also be installed.

```
git clone https://github.com/nRF24/RF24.git
cd RF24/
make clean
./configure --driver=SPIDEV; make -j4; sudo make install
```

Once these dependencies are satisifed, build using the standard cmake flow.

```
git clone git@github.com:aarossig/nerfnet.git
cd nerfnet
mkdir build
cd build
cmake ..
make -j`nproc`
```

## network design

```
TODO(aarossig): Provide a high-level design description.
```

### link 

The link layer is the lowest layer in the network stack. The link layer can be
implemented using any media including wireless (NRF24L01) or wired (ethernet)
or even carrier pidgeon (subject to latency requirements of the network).

The link layer is intentionally left abstract to accomodate different media.

### transport

The transport layer is the framing layer in the network stack. The transport is
implemented in terms of a given link to support spliting larger data frames over
links that have smaller packet sizes.
