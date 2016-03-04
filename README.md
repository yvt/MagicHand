MagicHand
=========

![](https://dl.dropboxusercontent.com/u/37804131/github/MagicHand.gif)

MagicHand is a software that lets you control your computer using your smartphone or tablet, 
just by connecting them to the same network, and opening the displayed URL.

To feed the mouse pointer location, MagicHand uses WebSockets which allow the bidirectional communication between the browser and server.

Installation
----

1. Run `qmake`.
2. Run `make`.
3. Run `./MagicHand`.
4. Main window appears, in which a URL is shown. 
5. Open the URL with your smartphone.
6. Passcode will be displayed on your smartphone. Type it in the main window.
7. You have to calibrate the position. Point your smartphone toward the center of the screen and tap the "Calibrate" button.
8. Now you can control the mouse pointer with your smartphone.

Requirements
----

* Currently, MagicHand only supports Mac OS X. Other platforms are yet to be supported. (Pull requests are always welcome!)
* Smartphone or tablet with a gyroscope sensor.

Restrictions
----

* Precision is limited by the gyroscope sensor of your device.
* You might experience an observable lags.
* Occasional disconnection might occur. MagicHand has an ability to detect and recover from this situation.
* Simulating mouse button click other than the left mouse button is not supported.
* TLS (or aother security system) is yet to be supported. This means you have to use a encrypted network or your computer might get controlled by someone.


