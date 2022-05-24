# microTCP
A lightweight TCP implementation using UDP transport layer.

This is the class project for CS-335a (www.csd.uoc.gr/~hy335a/) for the
Fall 2017 semester.

What could we have done better:
  - Flow Control
  - Fast Retransmission
  - Retransmission on wrong checksum
## Build requirements
To build this project `cmake` is needed.

## Build instructions
```bash
mkdir build
cd build
cmake ..
make
```