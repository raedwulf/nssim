NS3 Simulation Example
======================

Building
--------

$ cd nssim
$ mkdir -p build
$ cd build
$ cmake ..
$ make

Running
-------
This generates a wifisim executable that can be run through playernsd

Assuming you are the playernsd directory
	$ playernsd pathto/nssim/build/wifisim

This will pass all messages through the wifisim executable.
