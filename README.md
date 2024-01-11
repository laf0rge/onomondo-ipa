# Onomondo IPAd implementation

This repository contains the [current state of work towards] an IPAd
implementation.

The IPAd (IoT Profile Assistance [device]) is an element in the 3GPP IoT eSIM
system as described in SGP.31 and SGP.32.  It interfaces between the eUICC
on the one hand side, and the eIM (via HTTPS) on the other side.

## Dependencies

The IPAd core implementation (libasn, libipa) written in a way so that it has
no dependencies other than a c99 compliant C-compiler. However, the IPAd still
requires platform dependent modules that allow it to make HTTP(s) requests and
to access the eUICC via some sort of smartcard reader. This repository ships
with a sample implementation those platform depended modules that can run
on a standard Linux system:

* `http.c`: Contains a libcurl based implementation to make HTTP(s) requests.
* `scard.c`: Contains a libpcsclite based implementation to access the eUICC.

On a Debian GNU/Linux system the following packages must be installed:

* libcurl4-gnutls-dev
* libpcsclite-dev
* build-essential
* cmake

you can use the standard `apt-get install ...` command to install those dependencies.


## Sample application

Along with the platform depended modules also comes a sample application
(`main.c`). The sample application is a fully functional IPAd that runs on a
linux system. However, its main purpose is to illustrates how to use the API
presented in `onomondo/ipad.h`.


## Building from source

To compile the IPAd the run the following four steps:

```
mkdir build
cd build
cmake -DENABLE_SANITIZE=ON ../
make
```

The option `-DENABLE_SANITIZE` is entirely optional and results in a build
with AddressSanitizer which helps to find out-of-bounds memory accesses
during development and testing.

## Running

The resulting IPAd binary can be executed without parameters:

```
./src/ipa/ipa
```

The IPAd will then read the first eIM from the eIM configuration of the eUICC
and poll the eIM (GetEimPackage) exactly once. The user also has the option to
specify some basic parameters from the command-line. (use the -h option to get
a list)
