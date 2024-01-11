#!/bin/bash

cd ../src/ipa/libasn/

# Remove an old implementation
rm -f *.c
rm -f *.h

# Compile ASN.1 specification to actual C implementation
asn1c -fcompound-names -no-gen-example \
      ../../../asn1/PKIX1Explicit88.asn \
      ../../../asn1/PKIX1Implicit88.asn \
      ../../../asn1/PEDefinitions.asn \
      ../../../asn1/RSPDefinitions.asn \
      ../../../asn1/SGP32Definitions.asn

# Remove file(s) we do not need
rm ./Makefile.am.libasncodec

# Generate CMakeLists.txt
echo "#CAUTION: autgenerated file, do not change, see "`basename $0` > CMakeLists.txt
echo 'add_library(libasn STATIC' >> CMakeLists.txt
ls *.h *.c -1 >> CMakeLists.txt
echo ')' >> CMakeLists.txt
echo 'target_include_directories(libasn PUBLIC ${CMAKE_SOURCE_DIR}/include PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})' >> CMakeLists.txt
