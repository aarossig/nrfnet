# nerfnet

```
TODO(aarossig): Update this readme with new mesh documentation.
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
