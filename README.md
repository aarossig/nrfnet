# nerfnet

An application that allows using nRF24L01 radios to establish a wireless network
between Raspberry Pi computers.

## design

This application uses virtual network devices to create tunnels on each system
that runs `nerfnet`. The radios are run in either primary or secondary mode. It
doesn't matter which radio is primary and which is secondary. This merely
determines which system initiates the request to exchange network packets.

This makes the design polled from the primary radio to the secondary radio.

## building

This project uses the cmake build system. It also uses protobuf for the messages
exchanged over the NRF24L01 radios. The following packages must be installed:

```
sudo apt-get install git cmake build-essential \
    libtclap-dev \
    libprotobuf-dev protobuf-compiler
```

Once the required packages are installed, the standard cmake workflow is used:

```
git clone git@github.com:aarossig/nerfnet.git
cd nerfnet
mkdir build
cd build
cmake ..
make -j4
```

Watch for any errors after running cmake to check for mising packages.

## usage

As mentioned above, `nerfnet` relies on polling from a primary radio to a
secondary radio. Here are example invocations for the primary and secondary
radios to establish the network link.

The `ip` command is used to assign IP addresses to allow these devices to
communicate over the network.

### primary

```
sudo ./nerfnet/net/nerfnet --primary
sudo ip addr add 192.168.10.1/24 dev nerf0
```

### secondary

```
sudo ./nerfnet/net/nerfnet --secondary
sudo ip addr add 192.168.10.2/24 dev nerf0
```

## testing

Once the link is established, any standard networking tools can be used to
characterize the link. Here is an example of using `iperf`.

```
# On the primary Raspberry Pi.
iperf -s
# On the secondary Raspberry Pi.
iperf -c 192.168.10.1
```

This will evaluate the link performance.

Any other network applications can be used over this link such as `ssh` or
otherwise.
