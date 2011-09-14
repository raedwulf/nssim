NS3 Simulation Example
======================

Introduction
------------

This is a simple example for use with PlayerNSD to use ns3 as the backend for simulation.

Dependencies
------------

* [CMake][1] (tested with 2.8.1)
* [PlayerNSD][2] (tested with 0.0.1)
* [NS3][3] (tested with tip)

 [1]: http://github.com/raedwulf/playernsd
 [2]: http://www.cmake.org/
 [3]: http://www.nsnam.org/

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
	$ ./playernsd pathto/nssim/build/wifisim

This will pass all messages through the wifisim executable.
