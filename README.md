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

This project uses the cmake build system and tclap for command-line arguments.
The following packages must be installed:

```
sudo apt-get install git cmake build-essential \
    libtclap-dev
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

### primary

```
sudo nerfnet --primary
```

### secondary

```
sudo nerfnet --secondary
```

### flags

There are a number of flags to configure the radios. They can all be obtained
by passing `--help`.

#### ip address

`nerfnet` will automatically assign `192.168.10.1` to the primary radio and
`192.168.10.2` to the secondary radio. This can be overridden with the
`--tunnel_address` and `--tunnel_mask` options. Examples:

```
sudo nerfnet --primary --tunnel_address 192.168.100.1
```

```
sudo nerfnet --secondary --tunnel_address 10.0.0.1 --tunnel_mask 255.0.0.0
```

#### channel

The NRF24L01 radios have 128 channels to choose from (0 to 127). It may be
helpful to adjust the channel if a segment of the 2.4GHz band is congested.
These radios default to channel 1.

```
sudo nerfnet --primary --channel 10
```

#### poll interval (primary only)

The primary radio polls the secondary radio to simplify the interaction
between nodes. The secondary is always queuing packets and waits for the
primary radio to request them. The poll interval defaults to 100 microeconds.
In order to save CPU time and reduce traffic on the air, this can be adjusted.

```
sudo nerfnet --primary --poll_interval_us 1000
```

The longer the interval, the higher the latency and throughput of the link.
This may be desirable for certain applications, so this is left configurable.

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

## trivia

This README was written using an SSH connection that was established over a
`nerfnet` wireless link.

This protocol is vulnerable to pretty much every attack known to exist. Here
are the vulnerabilities that I can think of.

1) No validation of nodes (lack of signing).
2) No encryption.
3) Subject to replay/timing attacks.

The nice thing about widespread odoption of TLS these days is that these
vulnerabilities become less critical. Unencrypted traffic is vulnerable
to eavesdropping and manipulation.

It is also possible that if two users of `nerfnet` reside on the same channel
that their packets will cause loads of errors for each other. It would be
best to avoid that by selecting different channels.

Maybe don't use this for anything too important, or do ;)

Enjoy!
