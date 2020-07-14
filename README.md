# v2x

Visualizer for V2X CAM, DENM, MAPEM and SPATEM messages.
Tested on macOS 10.14.6 and ArchLinux.

## Dependencies
1. SFML
2. asn1c compiled and installed from [this](https://github.com/velichkov/asn1c) fork (s1ap branch)
3. cmake (CMakeLists specifies 3.13 or greater but lower should work, just change it)
4. libpcap
5. A c++20 compatible compiler, maybe. I specify c++20 in the build options but I don't know if I actually use any features of c++20.
6. (Optional) A running itsg5proxy that is spitting out raw ethernet frames

### macOS
You can use homebrew to install the dependencies.

## How to build
The main buildsystem is CMake but the ASN.1 compiler is invoked from make. make also invokes CMake but you can do it manually.

```bash
make asn
make
```

## How to run
Run from main directory as it needs to find the font.
You have to specify the IP, *not* the hostname!
If you don't have an itsg5proxy to connect to, just don't give it an IP.

```bash
./app [<ip of itsg5proxy>]
```

## External files
ASN.1 files have been obtained from [here](https://forge.etsi.org/rep/LIBS/LibIts/tree/STF525/asn1) and Wireshark.

You can get additional pcap for replay here:

Desc | Link
--- | ---
2020-07-14 8:50 until 18:00 | https://weichbrodt.me/files/2020-07-14-bs-muehlenpfordt-rebenring.pcapng
