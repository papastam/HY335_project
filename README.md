# microTCP
A lightweight TCP implementation using UDP transport layer.

This is the class project for [HY335a](www.csd.uoc.gr/~hy335a/) for the
Fall 2021 semester.

The microTCP implementation is included in the `./lib` directory

What could we have done better:
  - Flow Control
  - Fast Retransmission
  - Retransmission on wrong checksum

## Build Instructions
*To build the project `cmake` is needed.*

```bash
mkdir build
cd build
cmake ..
make
```

## Running Insttructions

The test files are generated in the `./build/test` directory after building.

The testfiles contained are:
+ `test_microtcp_client` *A simple cient application which sends a given file to the microtcp server*
+ `test_microtcp_server` *A simple server application which recieves a tcp connection and data*
+ `traffic_generator_client` *Generate traffic to a server using our microtcp* 

## Coowners
[Orestis Chiotakis](https://github.com/chiotak0) <br>
[Dimitris Bisias](https://github.com/dbisias)
