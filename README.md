NS3 Simulation Example
======================

Introduction
------------

This is a simple example for use with PlayerNSD to use ns3 as the
backend for simulation.

Dependencies
------------

* [CMake][1] (tested with 2.8.1)
* [PlayerNSD][2] (tested with 0.0.1)
* [NS3][3] (tested with tip)

 [2]: http://github.com/raedwulf/playernsd
 [1]: http://www.cmake.org/
 [3]: http://www.nsnam.org/

Building NS3
------------

Instructions on how to build NS3 from the mercurial tip are available [here][4].

 [4]: http://www.nsnam.org/wiki/index.php/Installation

A quick start for those on Linux systems,

```bash
	$ cd $BUILDDIR
	$ hg clone http://code.nsnam.org/ns-3-allinone
	$ cd ns-3-allinone
	$ python download.py -n ns-3-dev
	$ python build.py --enable-examples
	$ cd ns-3-dev
	$ ./waf install --prefix=$INSTALLPATH --destdir=$INSTALLPATH
```

This assumes ``$BUILDDIR`` and ``$INSTALLPATH`` are defined as being the build
directory and the target install directory respectively.

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
