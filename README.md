MagicHand
=========

![](https://dl.dropboxusercontent.com/u/37804131/github/MagicHand.gif)

MagicHand is a software that lets you control your computer using your smartphone or tablet, 
just by connecting them to the same network, and accessing the displayed URL.

MagicHand uses WebSocket, which is a recent web technology that allows webpages to communicate
with a server in a more interractive way, to transmit the mouse pointer location and other
signals.

Installation
----

1. `qmake`
2. `make`
3. './MagicHand'
4. Now unleash your smartphone's power

TODO: more details

Requirements
----

* Currently, MagicHand only supports Mac OS X. Other platforms are yet to be supported.
* Smartphone or tablet which have a gyroscope sensor.

Restrictions
----

* Precision is limited by the gyroscope sensor of your device.
* You might experience an observable lags, even when ping RTT is lower than 50ms.
* Simulating mouse button click other than the left mouse button is not supported.
* TLS (or aother security system) is yet to be supported. Currently you have to use a secure network.





