# VWAP Algorithmic Trading Client (ARC)

To start a Market Data Streaming Server:

Supply a Port and the Mode (0 = market data server)

```
./start_server 8081 0
```

To start an Order Placement Server:

Supply a Port and the Mode (1 = order placement server)

```
./start_server 9091 1
```

To start the algorithm:

Supply an IP Address and 2 Ports (market data server, quote acceptor)

```
./start_arc 127.0.0.1 8081 9091
```

Example Output

```
Connecting to 127.0.0.1
vwap = 9223372036854775807
piping order data
t> 152bdb59d3483af9 XYZ.USD $    1099 x  10
t> 152bdb59d8546fd2 BTC.USD $  991384 x   9
t> 152bdb59dc1a5b66 XYZ.USD $    1046 x   6
t> 152bdb59dcd6b300 BTC.USD $  991240 x   4
q> 152bdb59e2bff9d6 BTC.USD $  991232 x   5, $  991292 x   5
t> 152bdb59e46efe5d XYZ.USD $     922 x  11
t> 152bdb59e6fe6c69 ETH.USD $   82008 x   7
q> 152bdb59eaa60ed1 BTC.USD $  991226 x  12, $  991257 x   9
q> 152bdb59eb615063 BTC.USD $  991231 x   6, $  991300 x   8
t> 152bdb59eb927874 BTC.USD $  991390 x   6
q> 152bdb59f0917c98 BTC.USD $  991222 x   7, $  991278 x  10
t> 152bdb59f66d87a5 XYZ.USD $    1009 x   8
q> 152bdb59f6eeac18 BTC.USD $  991259 x   7, $  991265 x   7
q> 152bdb59fa1b32a1 BTC.USD $  991257 x   3, $  991279 x   2
q> 152bdb59fabe2958 BTC.USD $  991234 x  11, $  991248 x   8
t> 152bdb59fc5043cf XYZ.USD $     997 x   6
t> 152bdb59feb65ac3 ETH.USD $   82119 x   6
t> 152bdb5a012b8686 ETH.USD $   82017 x  10
q> 152bdb5a022b0611 BTC.USD $  991256 x   9, $  991268 x   2
t> 152bdb5a066c729d XYZ.USD $    1010 x   1
t> 152bdb5a0c481f89 XYZ.USD $    1019 x  10
t> 152bdb5a0d128171 ETH.USD $   82081 x  11
vwap = 99135557
q> 152bdb5a0f1c2832 BTC.USD $  991268 x   5, $  991298 x  11
q> 152bdb5a0f7ad0af BTC.USD $  991276 x  12, $  991299 x   8
t> 152bdb5a1565b992 ETH.USD $   81986 x   4
t> 152bdb5a19c3df53 BTC.USD $  991286 x   2
t> 152bdb5a1eab874f BTC.USD $  991526 x   2
t> 152bdb5a246652c8 XYZ.USD $    1049 x   3
t> 152bdb5a26fa83d9 ETH.USD $   82129 x   1
q> 152bdb5a2cb838b9 BTC.USD $  991226 x   4, $  991242 x  13
q> 152bdb5a2e3d92f2 BTC.USD $  991259 x   5, $  991249 x  10
t> 152bdb5a3428ac89 BTC.USD $  991502 x  10
q> 152bdb5a365f38ec BTC.USD $  991237 x   4, $  991291 x   3
q> 152bdb5a3a147804 BTC.USD $  991277 x   5, $  991271 x   8
t> 152bdb5a3e53f4f7 XYZ.USD $    1089 x  11
t> 152bdb5a443188cc ETH.USD $   82136 x   2
q> 152bdb5a44822f37 BTC.USD $  991251 x   5, $  991298 x  10
t> 152bdb5a480c0326 ETH.USD $   82142 x   5
t> 152bdb5a4a0a8e08 ETH.USD $   82020 x   5
```
