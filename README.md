# DimonneTalk

## Synopsis
This is a Telegram bot for translating normal text to Dimonne notation.

## Dependencies
* C++ 14 capable C++ compiler (currently tested with GCC 6.3.0)
* [cmake](https://cmake.org/)
* Development version of [Boost libraries](http://www.boost.org/), which includes not yet released boost.beast HTTP library
* [OpenSSL](https://www.openssl.org/)
* [Lua](https://www.lua.org/) >= 5.3

### Building dependencies
Here are steps for building **DimonneTalk** on Unix:

* Clone and build developoment version of Boost:

```bash
git clone https://github.com/boostorg/boost.git
cd boost
git submodule update --init
./bootstrap.sh --prefix="`pwd`/build"
./b2 headers
./b2 # Add -jN option to speed up the build where N is the number of concurrent build jobs
./b2 install
```

* Clone and build OpenSSL:

```bash
git clone https://github.com/openssl/openssl.git
cd openssl
git checkout OpenSSL_1_1_0g # Latest release at the time of writing
./Configure --prefix="`pwd`/build" linux-x86_64
make # Add -jN option to speed up the build where N is the number of concurrent build jobs
make install
```
