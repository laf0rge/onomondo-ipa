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
cmake -DENABLE_SANITIZE=ON -DSHOW_ASN_OUTPUT=ON ../
make
```

The option `-DENABLE_SANITIZE` is entirely optional and results in a build
with AddressSanitizer which helps to find out-of-bounds memory accesses
during development and testing.

The option -DSHOW_ASN_OUTPUT enables decoded printing of the ASN.1 encoded
messages that are exchanged between eIM and eUICC. The decoded ASN.1 output
may result in large log output, so it is recommended to use this option
only for development/testing. (The hexadecimal representation of messages is
still printed)

The code that is used to encode/decode ASN.1 encoded messages has been
generated using asn1c. This ASN.1 compiler also adds debug messages, which can
be enabled by adding the option -DASN_EMIT_DEBUG=ON.

To debug the usage of heap memory the option -DMEM_EMIT_DEBUG=ON can be used.
When this option is enabled IPA_ALLOC, IPA_ALLOC_N, IPA_REALLOC, and IPA_FREE
will keep track on how much memory is currently allocated. The current memory
usage and the peak memory usage is then displayed. The feature relys on the
function malloc_usable_size(), which is a non standard API. However, the
function is available on GNU LINUX and FreeBSD (see also man malloc_usable_size)

## Running

The resulting IPAd binary can be executed without parameters:

```
./src/ipa/ipa
```

The IPAd will then read the first eIM from the eIM configuration of the eUICC
and poll the eIM (GetEimPackage) exactly once. The user also has the option to
specify some basic parameters from the command-line. (use the -h option to get
a list)
