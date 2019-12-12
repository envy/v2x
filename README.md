v2x
===


Dependencies
------------

1. SFML
2. asn1c compiled and installed from [this](https://github.com/velichkov/asn1c) fork
3. cmake
4. A running itsg5proxy that is spitting out raw ethernet frames


How to build
------------

The main buildsystem is CMake but the ASN.1 compiler is invoked from make. make also invokes CMake.

```bash
make asn
make
```

How to run
----------

Run from main directory as it need to find the font.
You have to specify the IP, *not* the hostname!

```bash
./app <ip of itsg5proxy>
```
