# VWAP Algorithmic Trading Client (ARC)

To start a Market Data Streaming Server:

Supply a Port and the Mode (0 = market data server)

```
./start_server 8081 0
```

To start the algorithm:

Supply an IP Address and 2 Ports (market data server, quote acceptor)

```
./start_arc 127.0.0.1 8081 9091
```
