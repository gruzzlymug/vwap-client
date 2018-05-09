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

Example Output

```
Accepting Orders
o> 9adf58d83d295 BTC.USD S $  991580 x    8
o> 9adf5b85b10e8 BTC.USD S $  991569 x   10
o> 9adf61e00e7e0 BTC.USD S $  991605 x    4
```

To start the algorithm:

Supply an IP Address and 2 Ports (market data server, order placement server)

```
./start_arc 127.0.0.1 8081 9091
```

Example Output

```
❯ ./start_arc 127.0.0.1 8081 9091                                                                                               
Connecting to 127.0.0.1
Connecting to 127.0.0.1
INITIALIZING
0> 9adf473b2f319 9adf473b2f319 BTC.USD   991553    1
1> 9adf473b2f319 9adf57d44e089 BTC.USD   991567    2
READY
vwap = 99156277
q> 9adf5830bf065 BTC.USD $  991549 x   1, $  991569 x  13
q> 9adf58d83d295 BTC.USD $  991580 x   8, $  991600 x  10
SELL: -1723
>> 9adf58d83d295 BTC.USD S $991580 x 8
.....
vwap = 99156405
.....
.....
.....
.....
.....
.....
q> 9adf5b85b10e8 BTC.USD $  991569 x  10, $  991589 x   2
SELL: -495
>> 9adf5b85b10e8 BTC.USD S $991569 x 10
vwap = 99156464
.....
.....
vwap = 99156550
.....
q> 9adf5ef37ddb9 BTC.USD $  991534 x  10, $  991554 x   5
q> 9adf5f3f7718d BTC.USD $  991551 x  12, $  991571 x   5
vwap = 99156603
q> 9adf6087144ff BTC.USD $  991544 x  13, $  991564 x  13
vwap = 99156568
q> 9adf618422d39 BTC.USD $  991546 x  13, $  991566 x   8
q> 9adf61e00e7e0 BTC.USD $  991605 x   4, $  991625 x  11
SELL: -3932
>> 9adf61e00e7e0 BTC.USD S $991605 x 4
.....
.....
.....
vwap = 99156717
.....
.....
.....
```
